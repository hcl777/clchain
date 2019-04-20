#include "httprsp.h"
#include "util.h"
#include "net.h"
#include "urlcode.h"
#include "HttpResponseHeader.h"
#include "HttpContentType.h"
#include "file64.h"

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

bool g_https_exiting=false;
using namespace cl;
httprsp::httprsp(cl_httppubconf_t* c)

{
	m_req = new HttpRequest_t();
	m_req->fun_param = c->fun_param;
	m_pubconf = c;
}

httprsp::~httprsp(void)
{
	delete m_req;
}
int httprsp::handle_req(SOCKET sock,sockaddr_in &addr)
{
	HttpRequest_t* req = m_req;
	req->fd = (int)sock;
	req->addr.s_addr = addr.sin_addr.s_addr;
	int timeoutms = 8000;
#ifndef ANDROID
	cl::net::sock_set_timeout((int)sock, 8000);
#endif
	do
	{
		//只支持Get 方法头，不支持头含数据，也就是收到"\r\n\r\n"后认为是一个完整请求
		req->reset();

		if(0!=recv_head(req,timeoutms))
			break;

		//如果body数据比较少，则收完它
		if(req->Content_Length>req->bodylen && req->Content_Length<=CL_HTTP_MAX_HEADLEN)
		{
			if(0!=cl::net::sock_select_recv_n(req->fd,req->body+req->bodylen,(int)(req->Content_Length-req->bodylen),timeoutms))
				break;
			req->bodylen = (int)req->Content_Length;
		}
		req->body[req->bodylen] = '\0';
		
		if(0!=handle_head(req, m_pubconf->fun_handle_http_req)) //返回0才表示正确完整处理，才可能继续循环处理(如果keepalive)
			break;
	}while(m_pubconf->bsupport_keepalive && is_keeplive(req->header));
	return 0;
}
int httprsp::recv_head(HttpRequest_t* req,int timeoutms)
{
	int n = 0;
	int readsize = 0;
	uint64_t begintick = mtick();
	char *p = NULL;
	char* buf = req->header;
	while(readsize<CL_HTTP_MAX_HEADLEN)
	{
		if(1==cl::net::sock_select_readable(req->fd,timeoutms))
		{
			n = recv(req->fd,buf+readsize,CL_HTTP_MAX_HEADLEN-readsize,0);
			if(n<=0)
			{
				break;
			}
			readsize+=n;
			buf[readsize] = '\0';
			p = strstr(buf,"\r\n\r\n");
			if(p)
			{
				req->bodylen = readsize - (int)(p-buf+4);
				assert(req->bodylen>=0);
				if(req->bodylen>0)
				{
					memcpy(req->body,p+4,req->bodylen);
				}
				p[4] = '\0';
				return 0;
			}
		}

		//超时 10秒
		if(time_after(mtick(),(begintick + timeoutms)))
			break;
	}
	return -1;
}
int httprsp::handle_head(HttpRequest_t* req,FUN_HANDLE_HTTP_REQ_PTR fun)
{
	//DEBUGMSG("request------- \n %s \n",header);
	string text = "";
	if(0==get_header_field(req->header,"Content-Length",text))
	{
		req->Content_Length = cl::atoll(text.c_str());
	}
	char buf[CL_HTTP_MAX_HEADLEN+1];
	char* ptr = strchr(req->header,'\r');
	if(NULL==ptr) return -1;
	memcpy(buf,req->header,(int)(ptr-req->header));
	buf[(int)(ptr-req->header)] = '\0';
	string src = buf;
	string str;
	str = cl::get_string_index(src,0," ");
	if(str.empty()||str.length()>7) return -1;
	strcpy(req->method,str.c_str());
	str = cl::get_string_index(src,1," ");
	if(str.empty()||str.length()>CL_HTTP_MAX_HEADLEN) return -1;
	str = cl::urldecode(str);
	int pos = (int)str.find("?");
	if(pos>0)
	{
		strcpy(req->cgi,str.substr(0,pos).c_str());
		strcpy(req->params,str.substr(pos).c_str());
	}
	else if(pos==0)
	{
		return -1;
	}
	else
	{
		strcpy(req->cgi,str.c_str());
		req->params[0]='\0';
	}

	if(req->cgi == strstr(req->cgi,"/version") && !m_pubconf->ver.empty())
	{
		char rspbuf[1024];
		sprintf(rspbuf, "ver=%s<br>"
			"begintime=%s<br>"
			"bmulti_thread=%d<br>"
			"bsupport_keepalive=%d<br>"
			"max_client_num=%d<br>"
			"current_clients=%d<br>"
			"request_amount=%d<br>"
			, m_pubconf->ver.c_str(), m_pubconf->begin_time.c_str(), m_pubconf->multi_thread?1:0,
			m_pubconf->bsupport_keepalive?1:0, m_pubconf->max_client_num,
			m_pubconf->client_num, m_pubconf->request_amount);
		response_message(req->fd, rspbuf);
		return 0;
	}
	else if (req->cgi == strstr(req->cgi, "/myip"))
	{
		//返回ip4字符串
		char sip[128];
		memset(sip, 0, 128);
		inet_ntop(AF_INET, (void*)&req->addr, sip, 128);
		response_message(req->fd, sip);
		return 0;
	}
	else if(fun)
	{
		return fun(req);
	}
	else
	{
		response_error((int)req->fd);
		return -1;
	}
}
int httprsp::recv_body(HttpRequest_t* req, string& s, int timeoutms)
{
	int size = (int)req->Content_Length;
	if (size > 10240000 || size <= 0)
		return -1;
	
	s.resize(size);
	char* p = (char*)s.data();
	if (req->bodylen > 0)
	{
		memcpy(p, req->body, req->bodylen);
		p += req->bodylen;
		size -= req->bodylen;
	}
	if (0 != cl::net::sock_select_recv_n(req->fd, p, size, timeoutms))
		return 1;
	return 0;
}
int httprsp::get_header_field(const string& header,const string& session, string& text)
{
	//取得某个域值,session 不带":"号
	if(header.empty()) 
		return -1;
	int nPos = -1;
	//nPos = (int)header.find(session,0); //忽略大小写查找。
	const char *p = cl::stristr(header.data(), session.data());
	nPos = p?(int)(p - header.data()):-1;
	if(nPos != -1)
	{
		nPos += (int)session.length();
		nPos += 1; //加1忽略:号
		int nCr = (int)header.find("\r\n",nPos);
		text = header.substr(nPos,nCr - nPos);
		cl::string_trim(text);
		return 0;
	}
	else
	{
		return -1;
	}
}
int httprsp::get_header_range(const string& header,uint64_t& ibegin,uint64_t& iend)
{
	string str;
	if(0==get_header_field(header,"Range",str))
	{
		str = cl::get_string_index(str,1,"=");
		ibegin = cl::atoll(cl::get_string_index(str,0,"-").c_str());
		iend = cl::atoll(cl::get_string_index(str,1,"-").c_str());
		return 0;
	}
	else
	{
		return -1;
	}
}
bool httprsp::is_keeplive(const string& header)
{
	//是否keep-alive
	string str;
	if(0==get_header_field(header,"Connection",str))
	{
		if(0==stricmp(str.c_str(),"keep-alive"))
		{
			DEBUGMSG("# httprsp::req \"keep-alive\" \n");
			return true;
		}
	}
	return false;
}

void httprsp::response_error(int fd,const char* msg/*=NULL*/,int code/*=404*/,int timeoutms/*=10000*/)
{
	printf("response %d",code);

	HttpResponseHeader responseHdr;

	string str = "error";
	if(msg)
		str = msg;

	responseHdr.AddStatusCode(code);
    responseHdr.AddDate();
    responseHdr.AddServer("sphttpsvr");
    responseHdr.AddMyAllowFields();
	responseHdr.AddContentLength((int)str.length());
    responseHdr.AddContentType("text/html");

	uint64_t tick = mtick();
	int distance = 0;
    if(responseHdr.Send(fd,timeoutms))
	{
		distance = (int)time_distance(mtick(),tick);
		if(timeoutms>= distance)
			cl::net::sock_select_send_n(fd,str.c_str(),(int)str.length(),timeoutms-distance);
	}
	cl::net::sock_select_readable(fd, 1000);
}
void httprsp::response_message(int fd,const char* msg,int len/*=-1*/,int code/*=200*/,int timeoutms/*=10000*/)
{
	HttpResponseHeader responseHdr;
	if(len<0)
	{
		if(msg)
			len = (int)strlen(msg);
		else
			len = 0;
	}
	if(0==len) code = 204;
    
	responseHdr.AddStatusCode(code);
    responseHdr.AddDate();
    responseHdr.AddServer("sphttpsvr");
    responseHdr.AddMyAllowFields();
	responseHdr.AddContentLength(len);
    responseHdr.AddContentType("text/html");

	uint64_t tick = mtick();
	int distance = 0;
    if(responseHdr.Send(fd,timeoutms))
	{
		if(len>0)
		{
			distance = (int)time_distance(mtick(),tick);
			if(timeoutms>= distance)
				cl::net::sock_select_send_n(fd,msg,len,timeoutms-distance);
		}
	}
	cl::net::sock_select_readable(fd, 2000);
}
void httprsp::response_message(int fd,const string& msg,int code/*=200*/,int timeoutms/*=10000*/)
{
	response_message(fd,msg.c_str(),(int)msg.length(),code,timeoutms);
}

void httprsp::response_file(int fd,const string& path)
{
	cl::file64 file;
	if(0!=file.open(path.c_str(),F64_READ))
	{
		printf("#***httpsvr no file (%s) ;\n",path.c_str());
		response_error(fd);
		return;
	}
	size64_t size = file.seek(0,SEEK_END);
	if(size<=0)
	{
		response_error(fd);
		return;
	}
	file.seek(0,SEEK_SET);

	HttpResponseHeader responseHdr;

	responseHdr.AddStatusCode(200);
	responseHdr.AddString("Access-Control-Allow-Origin: *\r\n");
    responseHdr.AddDate();
    responseHdr.AddServer("sphttpsvr");
    responseHdr.AddMyAllowFields();
	responseHdr.AddContentLength((long long)size);
	responseHdr.AddContentType(http_content_type[cl::substr_by_findlast(path,'.')]);
	responseHdr.Send(fd,10000);

	const int READLEN = 64 * 1024;
	char *buf=new char[READLEN];
	size64_t rdsize = 0;
	//
	int n = 0;
	while(rdsize<size)
	{
		n = file.read(buf, READLEN);
		if(n<=0)
			break;
		if(0!=cl::net::sock_send_n(fd,buf,n))
			break;
		rdsize += n;
	}
	delete[] buf;
	file.close();
	cl::net::sock_select_readable(fd,3000);//最多等10秒钟，当对方收完关闭时，会可读。
}

