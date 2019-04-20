#pragma once
#include "clreactor.h"

namespace cl
{
	class tcpacceptorHandle
	{
	public:
		virtual ~tcpacceptorHandle(void) {}
	public:
		virtual bool attach_tcp_socket(int fd, sockaddr_in& addr) = 0;
	};

	//tcpacceptor:
	class tcpacceptor : public rthandle
	{
	public:
		tcpacceptor();
		~tcpacceptor();

		int open(unsigned short port, const char* ip, tcpacceptorHandle* chf, reactor* reactor);
		void close();
		virtual int sock() { return (int)m_fd; }
		virtual int handle_input();
		virtual int handle_output() { return 0; }
		virtual int handle_error() { return 0; }

	private:
		tcpacceptorHandle * m_handle;
		SOCKET m_fd;
		reactor *m_reactor;
		GETSET(unsigned int, m_hip, _hip)
		GETSET(unsigned short, m_hport, _hport)
	};

}
