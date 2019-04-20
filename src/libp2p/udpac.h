#pragma once
#include "cltype.h"
#include "incnet.h"

namespace p2p {

	class udpac;
	class udpacHandler
	{
		friend class udpac;
	public:
		udpacHandler()
			:m_fd((int)INVALID_SOCKET)
		{}
		virtual ~udpacHandler() {}

	protected:
		int send(const char* buf, int size, sockaddr_in& addr) { return sendto(m_fd, buf, size,0,(sockaddr*)&addr,(int)sizeof(sockaddr_in)); }
		virtual int on_recv(char* buf, int size, sockaddr_in& addr) = 0;
	protected:
		int m_fd;
	};

	class udpac
	{
	public:
		udpac();
		~udpac();
	public:
		int open(unsigned short port, udpacHandler* table);
		int close();
		int read_loop(uint32_t delay_usec = 0);

	private:
		int				m_fd;
		udpacHandler*	m_table;
	
	};

}


