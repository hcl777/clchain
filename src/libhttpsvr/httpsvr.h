#pragma once

#include "httprsp.h"
#include "worker.h"
#include "semaphore.h"


class httpsvr : public cl::worker
{
public:
	httpsvr(void);
	~httpsvr(void);
	typedef struct tag_task_info
	{
		int state; //0ø’œ–,1”–»ŒŒÒ
		SOCKET s;
		sockaddr_in addr;
		httprsp* h;
		cl::semaphore sem;
	}task_info_t;

	

public:
	int open(unsigned short port,const char *ip=0,
		FUN_HANDLE_HTTP_REQ_PTR fun=0,void* fun_param=NULL,
		bool multi_thread=true,int max_client_num=20,
		const char* strver=NULL,bool bsupport_keepalive=true);
	int stop();
	bool is_open() const {return m_brun;}

	virtual void work(int e);

	const cl_httppubconf_t *get_conf() const { return &m_conf; }
	//int get_max_client_num()const { return m_max_client_num; }
	//int get_client_num()const {return m_client_num;}
	//int get_request_amount()const { return m_request_amount; }
	//int get_ip_num()const {return m_client_ips.size();}
private:
	int accpet_root();
	int get_idle();
	
	void add_client_ip(unsigned int nip);
	void del_client_ip(unsigned int nip);
private:
	char			m_ip[64];
	bool			m_brun;

	SOCKET				m_sock;
	task_info_t**	m_ti;
	cl_httppubconf_t	m_conf;
};

