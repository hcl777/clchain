#include "httpc.h"
#include "net.h"
#include "util.h"

CLBEGIN
namespace httpc
{
	int post(const string& url, const cl::clslice& body, string& rspbody)
	{
		response_t rsp;
		int ret = -1;
		int fd = open_request(url, body.data(), (int)body.size());
		if (fd == (int)INVALID_SOCKET)
			return -1;
		if (0 == recv_header(fd, &rsp) && 200 == rsp.retcode)
		{
			ret = recv_body(fd,&rsp, rspbody);
		}
		close_request(fd);
		return ret;
	}
	int get(const string& url, string& rspbody)
	{
		return post(url, cl::clslice(), rspbody); //空body就是get方法
	}

}
CLEND
