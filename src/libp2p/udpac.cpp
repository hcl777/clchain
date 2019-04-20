#include "udpac.h"
#include <assert.h>
#include "net.h"
#include <string.h>

namespace p2p {
	
	udpac::udpac()
		: m_fd((int)INVALID_SOCKET)
		, m_table(NULL)
	{
		
	}

	udpac::~udpac()
	{
	}
	int udpac::open(unsigned short port, udpacHandler* table)
	{
		assert(INVALID_SOCKET == m_fd);
		if (INVALID_SOCKET == (m_fd = cl::net::open_udp(port, 0, 1024000, 1024000)))
			return -1;

		cl::net::set_nonblock(m_fd, 1);

		m_table = table;
		m_table->m_fd = m_fd;
		return 0;
	}
	int udpac::close()
	{
		if (m_table)
			m_table->m_fd = (int)INVALID_SOCKET;
		::closesocket(m_fd);
		return 0;
	}

	int udpac::read_loop(uint32_t delay_usec/*=0*/)
	{
		char			_tmp_buf[4096];
		fd_set			_rfd;
		timeval			_timeout;
		int				_n;
		sockaddr_in		_tmp_addr;
		socklen_t		_tmp_addr_len;

		_tmp_addr_len = sizeof(sockaddr_in);
		memset(&_tmp_addr, 0, sizeof(_tmp_addr));

		_timeout.tv_sec = (long)(delay_usec / 1000000);
		_timeout.tv_usec = (long)(delay_usec % 1000000);
		FD_ZERO(&_rfd);
		FD_SET(m_fd, &_rfd);
		_n = select(m_fd + 1, &_rfd, NULL, NULL, &_timeout);
		if (_n > 0)
		{
			while (1)
			{
				_n = recvfrom(m_fd, _tmp_buf, 4096, 0, (sockaddr*)&_tmp_addr, &_tmp_addr_len);
				if (_n > 0)
				{
					m_table->on_recv(_tmp_buf, _n, _tmp_addr);
				}
				else
				{
					//在些判断错误类型,如果是缓冲区不够,尝试用更大的缓冲区收掉这个垃圾包.
					break;
				}
			}
			return 1;
		}
		return 0;
	}
}

