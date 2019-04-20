
#include "httpsvr.h"
#include "util.h"

//#ifdef _WIN32
//typedef int socklen_t;
//#endif

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif


httpsvr::httpsvr(void)
: m_brun(false)
, m_sock(INVALID_SOCKET)
, m_ti(NULL)

{
}

httpsvr::~httpsvr(void)
{

}
int httpsvr::open(unsigned short port,const char *ip/*=NULL*/,FUN_HANDLE_HTTP_REQ_PTR fun/*=0*/,void* fun_param/*=NULL*/,bool multi_thread/*=true*/,int max_client_num/*=20*/,const char* strver/*=NULL*/, bool bsupport_keepalive/* = true*/)
{
	if(m_brun)
		return -1;
	if(ip)
		strcpy(m_ip,ip);
	else
		memset(m_ip,0,64);

	m_conf.ver = strver;
	m_conf.begin_time = cl::time_to_datetime_string(time(NULL));
	m_conf.max_client_num = max_client_num;
	if(m_conf.max_client_num<1) m_conf.max_client_num=1;

	m_conf.fun_handle_http_req=fun;
	m_conf.fun_param = fun_param;
	m_conf.multi_thread = multi_thread;
	m_conf.bsupport_keepalive = bsupport_keepalive;
	m_sock = (int)socket(AF_INET,SOCK_STREAM,0);
	if(m_sock == INVALID_SOCKET)
	{
		DEBUGMSG("#: ***httpsvr::open() socket() faild!\n");
		return -1;
	}
	//设置端口重用
	int flag = 1;
	if(-1==setsockopt(m_sock,SOL_SOCKET,SO_REUSEADDR,(char*)&flag,sizeof(flag)))
	{
		perror("set reuseaddr faild!");
	}

	sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if(0==strlen(m_ip))
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		addr.sin_addr.s_addr = inet_addr(m_ip);

	if(SOCKET_ERROR == ::bind(m_sock,(sockaddr*)&addr,sizeof addr))
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		DEBUGMSG("#: ***httpsvr::open() bind() faild!\n");
		return -1;
	}

	if(SOCKET_ERROR == listen(m_sock,SOMAXCONN))
	//if(SOCKET_ERROR == listen(m_sock,5))
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		perror("#*** listen fail:");
		DEBUGMSG("#: ***httpsvr::open() listen() faild!\n");
		return -1;
	}

	m_brun = true;
	
	m_ti = new task_info_t*[m_conf.max_client_num];
	for(int i=0;i<m_conf.max_client_num;++i)
	{
		m_ti[i] = new task_info_t();
		m_ti[i]->state = 0;
		m_ti[i]->h = new httprsp(&m_conf);
	}

	activate(m_conf.max_client_num+1);
	
	DEBUGMSG("#httpsvr open(%s:%d, thread=%d)  \n",m_ip,port, m_conf.max_client_num);
	return 0;
}
int httpsvr::stop()
{
	if(!m_brun)
		return -1;
	m_brun = false;
	g_https_exiting = true;
	if(INVALID_SOCKET != m_sock)
	{
#ifndef _WIN32
		shutdown(m_sock,SHUT_RDWR);
#endif
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	for(int i=0;i<m_conf.max_client_num;++i)
		m_ti[i]->sem.signal();

	wait();

	for(int i=0;i<m_conf.max_client_num;++i)
	{
		if(m_ti[i]->state==1)
			closesocket(m_ti[i]->s);
		delete m_ti[i]->h;
		delete m_ti[i];
	}
	delete[] m_ti;

	return 0;
}
void httpsvr::work(int e)
{
	//
	//printf("httpsvr::work(%d)... \n",e);
	if(e== m_conf.max_client_num)
		accpet_root();
	else
	{
		while(m_brun)
		{
			m_ti[e]->sem.wait();
			if(0!=m_ti[e]->state)
			{
				add_client_ip(m_ti[e]->addr.sin_addr.s_addr);
				
				m_ti[e]->h->handle_req(m_ti[e]->s,m_ti[e]->addr);
				closesocket(m_ti[e]->s);
				
				del_client_ip(m_ti[e]->addr.sin_addr.s_addr);

				m_ti[e]->state = 0;
			}
			else
			{
				//DEBUGMSG("# *** no task(%3d) ***\n",e);
			}
		}
	}

}


int httpsvr::accpet_root()
{
	int i = -1;
	socklen_t addr_len = sizeof(sockaddr_in);
	while(m_brun && INVALID_SOCKET!=m_sock)
	{
		i = get_idle();
		if(i<0)
		{
			//DEBUGMSG("# *** full httprsp *** \n");
			cl::msleep(10);
			continue;
		}
		memset(&m_ti[i]->addr,0,sizeof(sockaddr_in));
		m_ti[i]->s = accept(m_sock,(sockaddr*)&m_ti[i]->addr,&addr_len);
		if(INVALID_SOCKET != m_ti[i]->s)
		{
			m_ti[i]->state = 1;
			m_ti[i]->sem.signal();
		}
	}
	return 0;

}
int httpsvr::get_idle()
{
	for(int i=0;i<m_conf.max_client_num;++i)
	{
		if(0==m_ti[i]->state)
			return i;
	}
	return -1;
}
void httpsvr::add_client_ip(unsigned int nip)
{
	//DEBUGMSG("# +++ new httprsp(client_num=%d) +++\n",m_client_num);
	cl::Guard l(m_conf.mt);
	m_conf.client_num++;
	m_conf.request_amount++;
	bool bfind = false;
	for(list<ipcount_t>::iterator it= m_conf.client_ips.begin();it!= m_conf.client_ips.end();++it)
	{
		ipcount_t& i = *it;
		if(i.nip == nip)
		{
			bfind = true;
			i.count++;
			break;
		}
	}
	if(!bfind)
	{
		ipcount_t i;
		i.nip = nip;
		i.count = 1;
		m_conf.client_ips.push_back(i);
	}
}
void httpsvr::del_client_ip(unsigned int nip)
{
	//DEBUGMSG("# --- end httprsp(client_num=%d) +++\n",m_client_num);
	cl::Guard l(m_conf.mt);
	m_conf.client_num--;
	for(list<ipcount_t>::iterator it= m_conf.client_ips.begin();it!= m_conf.client_ips.end();++it)
	{
		ipcount_t& i = *it;
		if(i.nip == nip)
		{
			i.count--;
			if(0==i.count)
				m_conf.client_ips.erase(it);
			break;
		}
	}
}

