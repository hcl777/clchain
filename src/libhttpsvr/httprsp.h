#pragma once
#include "incnet.h"
#include "cltype.h"
#include "net.h"
#include "guards.h"
#include <string>
#include <list>

using namespace std;

#define CL_HTTP_MAX_HEADLEN 2047

class httpsvr;
typedef struct tag_HttpRequest
{
	int fd;
	char header[CL_HTTP_MAX_HEADLEN+1];
	char cgi[CL_HTTP_MAX_HEADLEN+1];
	char params[CL_HTTP_MAX_HEADLEN+1];
	char method[8];  //GET,POST
	unsigned long long Content_Length;
	char body[CL_HTTP_MAX_HEADLEN+1];
	unsigned int bodylen;

	struct in_addr addr;
	void* fun_param; //应用程序参数

	tag_HttpRequest(void)
	{
		fd = 0;
		fun_param = NULL;

		reset();
	}
	void reset()
	{
		header[0] = '\0';
		cgi[0] = '\0';
		params[0] = '\0';
		method[0] = '\0';
		Content_Length = 0;
		body[0] = '\0';
		bodylen = 0;
	}
}HttpRequest_t;

typedef int (*FUN_HANDLE_HTTP_REQ_PTR)(HttpRequest_t*);

//计算IP数
typedef struct tag_ipcount
{
	unsigned int nip;
	int count;
}ipcount_t;
typedef struct tag_cl_httppubconf
{
	string ver; // /version返回
	string begin_time;

	bool			multi_thread;
	bool			bsupport_keepalive;
	int				max_client_num;

	cl::Mutex		mt;
	int				client_num;
	int				request_amount;//累计请求数
	list<ipcount_t>	client_ips;
	FUN_HANDLE_HTTP_REQ_PTR fun_handle_http_req;
	void*			fun_param;

	tag_cl_httppubconf(void)
		: multi_thread(true)
		, bsupport_keepalive(true)
		, max_client_num(20)
		, client_num(0)
		, request_amount(0)
		, fun_handle_http_req(NULL)
		, fun_param(NULL)
	{
	}
}cl_httppubconf_t;

class httprsp
{
public:
	httprsp(cl_httppubconf_t* c);
	~httprsp(void);

public:
	int handle_req(SOCKET sock,sockaddr_in &addr);

private:
	HttpRequest_t*			m_req;
	//FUN_HANDLE_HTTP_REQ_PTR		m_fun_handle_http_req;
	cl_httppubconf_t			*m_pubconf;

public:
	int recv_head(HttpRequest_t* req,int timeoutms);
	int handle_head(HttpRequest_t* req,FUN_HANDLE_HTTP_REQ_PTR fun);
	
	static int recv_body(HttpRequest_t* req,string& s, int timeoutms);

	static int get_header_field(const string& header,const string& session, string& text);
	static int get_header_range(const string& header,uint64_t& ibegin,uint64_t& iend);
	static bool is_keeplive(const string& header);

	static void response_error(int fd,const char* msg=NULL,int code=404,int timeoutms=10000);
	static void response_message(int fd,const char* msg,int len=-1,int code=200,int timeoutms=10000);
	static void response_message(int fd,const string& msg,int code=200,int timeoutms=10000);
	static void response_file(int fd,const string& path);

};
extern bool g_https_exiting;
