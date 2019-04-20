#pragma once

#include "cltype.h"

using namespace std;

CLBEGIN
namespace httpc
{

#define HTTPC_MAX_HEADLEN 2047
	typedef struct tag_response
	{
		char header[HTTPC_MAX_HEADLEN + 1];
		int retcode;
		unsigned long long Content_Length;
		char body[HTTPC_MAX_HEADLEN + 1]; //记录多数的数据
		unsigned long long bodylen;
		char *pbody;
		tag_response(void)
			:retcode(0)
			, Content_Length(0)
			, bodylen(0)
			, pbody(NULL)
		{
		}
		~tag_response(void)
		{
			if (pbody)
				delete[] pbody;
		}
	}response_t;
	typedef struct tag_urlinf
	{
		string server;
		string cgi;
		uint16_t port;
	}urlinf_t;

	int url_solve(const string& url, urlinf_t& ui);
	int format_header(string& header, const string& server, const string& cgi, int bodylen);
	int get_response_code(const char* rsphead);
	string& get_field(const char* header, const char* session, string& text);

	//
	int open_request(const string& url, const char* data, int datalen);
	int recv_header(int fd, response_t* rsp);
	int recv_body(int fd, response_t* rsp,string& res); //必须有Content-Length
	int close_request(int fd);
}
CLEND
