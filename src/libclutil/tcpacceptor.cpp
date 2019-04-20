#include "tcpacceptor.h"
#include "net.h"

namespace cl
{
	tcpacceptor::tcpacceptor(void)
		:m_handle(NULL)
		, m_fd(INVALID_SOCKET)
		, m_reactor(NULL)
	{
	}


	tcpacceptor::~tcpacceptor(void)
	{
	}
	int tcpacceptor::open(unsigned short port, const char* ip, tcpacceptorHandle* h, reactor* reactor)
	{
		assert(h&&reactor);
		assert(INVALID_SOCKET == m_fd);
		if (NULL == h|| NULL == reactor)
			return -1;
		close();

		m_reactor = reactor;
		m_handle = h;
		int flag = 0;
		do
		{
			m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (INVALID_SOCKET == m_fd)
			{
				flag = -1;
				break;
			}

#ifndef _WIN32
			////端口重用
			//int x = 1;
			//if(-1==setsockopt(m_fd,SOL_SOCKET,SO_REUSEADDR,(char*)&x,sizeof(x)))
			//{
			//	perror("setsockopt(reuseaddr)faild!");
			//}
#endif

			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			if (ip)
				addr.sin_addr.s_addr = net::aton(ip);
			else
				addr.sin_addr.s_addr = INADDR_ANY;

			if (SOCKET_ERROR == ::bind(m_fd, (sockaddr *)&addr, sizeof(addr)))
			{
				flag = 1;
				break;
			}

			if (SOCKET_ERROR == listen(m_fd, 5))
			{
				flag = 2;
				break;
			}

			if (NULL != m_reactor && 0 != m_reactor->register_handler(this, SE_READ))
			{
				flag = 3;
				break;
			}

			m_hport = port;
			if (ip)
				m_hip = net::atoh(ip);
			else
				m_hip = 0;
			DEBUGMSG("#tcpacceptor open (%s) \n", net::htoas(m_hip,m_hport).c_str());
		} while (0);

		if (0 != flag)
		{
			close();
			return -1;
		}
		return 0;
	}
	void tcpacceptor::close()
	{
		if (INVALID_SOCKET != m_fd)
		{
			if (m_reactor)
				m_reactor->unregister_handler(this, SE_READ);
			::closesocket(m_fd);
			m_fd = INVALID_SOCKET;
			m_hip = 0;
			m_hport = 0;
			m_reactor = NULL;
		}
		m_handle = NULL;
	}
	int tcpacceptor::handle_input()
	{
		//考虑epoll的边缘模型
		//DEBUGMSG("TCPAcceptor::handle_input()...\n");
		SOCKET fd;
		sockaddr_in addr;
		socklen_t len;
		while (1)
		{
			memset(&addr, 0, sizeof(addr));
			len = sizeof(addr);
			fd = accept(m_fd, (sockaddr*)&addr, &len);
			if (INVALID_SOCKET == fd)
				return 0;

			//如果频繁有相同IP连接进入，考虑洪水处理
			if (NULL == m_handle || !m_handle->attach_tcp_socket((int)fd, addr))
				::closesocket(fd);
		}
		return 0;
	}

}
