#pragma once
#include "clreactor.h"
#include "speaker.h"
#include "memblock.h"
#include <list>

namespace cl
{
	class tcpchannel;
	class tcpchannelListener
	{
	public:
		virtual ~tcpchannelListener() {}

		template<int I>
		struct S { enum { T = I }; };

		typedef S<1> Connected;
		typedef S<2> Disconnected;
		typedef S<3> Readable;
		typedef S<4> Writable;

		virtual void on(Connected, tcpchannel* ch) {}
		virtual void on(Disconnected, tcpchannel* ch) {}
		virtual void on(Readable, tcpchannel* ch, int* pwait) {}
		virtual void on(Writable, tcpchannel* ch) {}
	};

	class tcpchannel : public rthandle
		, public caller<tcpchannelListener>
	{
		typedef std::list<memblock*> SendList;
		typedef SendList::iterator SendIter;
	public:
		tcpchannel(reactor* rt);
		virtual ~tcpchannel();
	public:
		int attach( SOCKET s, sockaddr_in& addr);
		int connect(unsigned int ip, unsigned short port);
		int disconnect();
		int send(memblock *b);
		int recv(char *b, int size);

		virtual int sock() { return (int)m_fd; }
		virtual int handle_input();
		virtual int handle_output();
		virtual int handle_error();

	protected:
		virtual void reset();
		void close_socket();
		int on_connected();

	protected:
		SOCKET			m_fd;
		reactor*		m_rt;
		SendList		m_slist; //sending list
		int				m_smore; //recode last fire writable if call send();
		int				m_is_regwrite; //only be used after connected

		GET(int, m_state, _state)
		GET(uint64_t, m_last_active_tick, _last_active_tick)
		//GET(unsigned int, m_hip, _hip)
		//GET(unsigned short, m_hport, _hport)
		GET(endpoint_t, m_ep, _ep)
			
	};


}
