#pragma once
#include "bstream.h"
#include "tcpchannel.h"
#include "timer.h"

namespace cl
{
	class tcppeer;
	class tcppeerListener
	{
	public:
		virtual ~tcppeerListener() {}

		template<int I>
		struct S { enum { T = I }; };
		typedef S<1>	Connected;
		typedef S<2>	Disconnected;
		typedef S<3>	Packet;
		typedef S<4>	Writable;

		virtual void on(Connected, tcppeer* peer) {}
		virtual void on(Disconnected, tcppeer* peer) {}
		virtual void on(Packet, tcppeer* peer,uint8_t cmd, char* buf, int len) {}
		virtual void on(Writable, tcppeer* peer) {}
	};

	/*
	˵����tcppeer֧��9�ֽ�packetͷ��ʽ�������շ�,�ⲿָ������buffer
	*/
	class tcppeer : private tcpchannelListener
		, public caller<tcppeerListener>
		, public timerHandler
	{
	public:
		tcppeer(reactor* rt,timer* ti, memblockPool* pool,uint32_t nid,int max_rcvsize);
		virtual ~tcppeer();
	public:
		int attach(int fd, sockaddr_in& addr) { return m_ch.attach(fd, addr); }
		int connect(endpoint_t& ep) { return m_ch.connect(ep.ip, ep.port); }
		int disconnect();
		//int send(memblock* b) { return m_ch.send(b); } //��ֱ�ӷ���buffer
		void set_timeout(int tick); //��������Ӧ���ݳ�ʱ�Ͽ�
		virtual void on_timer(int e);
		tcpchannel* channel() { return &m_ch; }

	private:
		virtual void on(Connected, tcpchannel* ch);
		virtual void on(Disconnected, tcpchannel* ch);
		virtual void on(Writable, tcpchannel* ch);
		virtual void on(Readable, tcpchannel* ch, int* pwait);
		void on_data();
	public:
		template<typename T>
		int send_packet(uchar cmd, T& inf, int maxsize)
		{
			if (m_ch.get_state() != cl::CONNECTED) return -1;
			MBLOCK_NEW_RETURN_INT(m_pool,block, maxsize, -1)
			bstream ss(block->buf, block->buflen, 0);
			ss << m_nid;
			ss.skipw(4); //size
			ss << cmd;
			ss << inf;
			ss.fitsize32(4);
			block->wpos = ss.length();
			m_last_active_tick = mtick();
			return m_ch.send(block);
		}
		int send_nullpacket(uchar cmd, int maxsize)
		{
			if (m_ch.get_state() != cl::CONNECTED) return -1;
			MBLOCK_NEW_RETURN_INT(m_pool, block, maxsize, -1)
			bstream ss(block->buf, block->buflen, 0);
			ss << m_nid;
			ss.skipw(4); //size
			ss << cmd;
			ss.fitsize32(4);
			block->wpos = ss.length();
			m_last_active_tick = mtick();
			return m_ch.send(block);
		}
	private:
		tcpchannel		m_ch;
		timer*			m_timer; //�������timer,���������������
		memblockPool*	m_pool;
		memblock*		m_recvb;
		uint32_t		m_nid;
		uint64_t		m_last_active_tick;
		int				m_timeo;
		GETSET(void*, m_pdata,_pdata)
	};

}



