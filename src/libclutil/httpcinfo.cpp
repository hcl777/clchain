#include "httpcinfo.h"

#include "net.h"
#include "util.h"

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

CLBEGIN
namespace httpc
{

	int url_solve(const string& url, urlinf_t& ui)
	{
		//解析url
		string str;
		int pos = 0, pos2 = 0, pos3 = 0;
		ui.port = 80;
		ui.server = "";
		ui.cgi = "";

		pos = (int)url.find("http://", 0);
		if (pos >= 0)
			pos += 7;
		else
		{
			pos = (int)url.find("HTTP://", 0);
			if (pos >= 0)
				pos += 7;
			else
				pos = 0;
		}

		pos2 = (int)url.find(":", pos);
		pos3 = (int)url.find("/", pos);

		if (pos3 > 0 && pos2 > pos3)
			pos2 = -1;

		if (pos3 > pos)
		{
			if (pos2>pos)
			{
				ui.server = url.substr(pos, pos2 - pos);
				str = url.substr(pos2 + 1, pos3 - pos2 - 1);
				ui.port = atoi(str.c_str(),0);
			}
			else
			{
				ui.server = url.substr(pos, pos3 - pos);
			}
			ui.cgi = url.substr(pos3);
		}
		else
		{
			if (pos2>pos)
			{
				ui.server = url.substr(pos, pos2 - pos);
				str = url.substr(pos2 + 1);
				ui.port = atoi(str.c_str(),0);
			}
			else
			{
				ui.server = url;
			}
			ui.cgi = "/";
		}
		return 0;
	}
	int format_header(string& header, const string& server, const string& cgi, int bodylen)
	{
		char buf[2048];
		header = "";
		sprintf(buf, "%s %s HTTP/1.1\r\n", bodylen>0 ? "POST" : "GET", cgi.c_str());
		sprintf(buf + strlen(buf), "Host: %s\r\n", server.c_str());
		if (bodylen>0)
			sprintf(buf + strlen(buf), "Content-Length: %d\r\n", bodylen);
		//strcat(buf,"Accept: */*\r\n");
		//Accept-Encoding: gzip, deflate
		strcat(buf, "User-Agent: Mozilla/4.0 (compatible; httpc 1.0;)\r\n");
		strcat(buf, "Pragma: no-cache\r\n");
		strcat(buf, "Cache-Control: no-cache\r\n");
		//Content-Type: application/x-www-form-urlencoded ；表示普通表单格式上传
		//Content-Type: multipart/form-data  ；表示上传文件的规格
		//Content-Type: application/octet-stream ； 表示上传一个文件流数据
		strcat(buf, "Content-Type: application/x-www-form-urlencoded\r\n");
		//strcat(buf,"Connection: Close \r\n");
		strcat(buf, "\r\n");
		header = buf;
		return 0;
	}
	int get_response_code(const char* rsphead)
	{
		const char* p = strstr(rsphead, " ");
		if(p)  return cl::atoi(p+1,0);
		return -1;
	}
	string& get_field(const char* header, const char* session, string& text)
	{
		//取得某个域值,session 不带":"号
		//忽略大小写查找。
		int len;
		const char *p1, *p2;
		text.clear();
		p1 = cl::stristr(header, session);
		if (p1)
		{
			p1 += strlen(session) + 1;//加1忽略:号
			p2 = strstr(p1, "\r\n");
			if (p2)
			{
				len = (int)(p2 - p1);
				text.resize(len);
				memcpy((char*)text.data(), p1, len);
				cl::string_trim(text);
			}
		}
		return text;
	}

	int open_request(const string& url, const char* body, int bodylen)
	{
		urlinf_t ui;
		string header, ip;
		SOCKET sock = INVALID_SOCKET;
		//int err = 0;
		try
		{
			url_solve(url, ui);
			ip =net::ip_explain(ui.server);
			if (ip.empty()) throw(1);
			format_header(header, ui.server, ui.cgi, bodylen);
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (INVALID_SOCKET == sock)
				throw(2);
			net::sock_set_timeout((int)sock, 20000);

			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(ui.port);
			addr.sin_addr.s_addr = inet_addr(ip.c_str());
			if (SOCKET_ERROR == connect(sock, (sockaddr*)&addr, sizeof(addr)))
				throw(3);
			if (0 != net::sock_send_n((int)sock, header.c_str(), (int)header.length()))
				throw(4);
			if (bodylen>0 && 0 != net::sock_send_n((int)sock, body, bodylen))
				throw(5);

		}catch(int)
		{
			if (sock != INVALID_SOCKET)
			{
				closesocket(sock);
				sock = INVALID_SOCKET;
			}
			//printf("***http request faild! err=%d, url=%s \n",e, url.c_str());
		}
		return (int)sock;
	}
	int recv_header(int fd, response_t* rsp)
	{
		int n = 0;
		int readsize = 0;
		char *p = NULL;
		char* buf = rsp->header;
		bool bok = false;
		string str;
		rsp->bodylen = 0;
		while (readsize<HTTPC_MAX_HEADLEN)
		{
			n = recv(fd, buf + readsize, HTTPC_MAX_HEADLEN - readsize, 0);
			if (n <= 0)
			{
				return -1;
			}
			readsize += n;
			buf[readsize] = '\0';
			p = strstr(buf, "\r\n\r\n");
			if (p)
			{
				rsp->bodylen = readsize - (int)(p - buf + 4);
				assert(rsp->bodylen >= 0);
				if (rsp->bodylen>0)
				{
					memcpy(rsp->body, p + 4, (int)rsp->bodylen);
				}
				p[4] = '\0';
				bok = true;
				break;
			}
		}
		if (!bok)
			return -1;
		rsp->retcode = get_response_code(rsp->header);
		rsp->Content_Length = cl::atoll(get_field(rsp->header, "Content-Length", str).c_str());
		return 0;
	}
	int recv_body(int fd, response_t* rsp, string& res)
	{
		res.clear();
		if (0 == rsp->Content_Length)
			return 0;
		int size = (int)rsp->Content_Length;
		if (size > 10240000 ||size<0)
			return -1;

		res.resize(size);
		char* p = (char*)res.data();
		if (rsp->bodylen > 0)
		{
			memcpy(p, rsp->body, rsp->bodylen);
			p += rsp->bodylen;
			size -= (int)rsp->bodylen;
		}
		if (0 != cl::net::sock_recv_n(fd, p, size))
			return 1;
		return 0;
	}
	int close_request(int fd)
	{
		return closesocket(fd);
	}

}
CLEND
