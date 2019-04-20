#include "tcppeer.h"


namespace cl
{
	tcppeer::tcppeer(reactor* rt, timer* ti, memblockPool* pool,uint32_t nid,int max_rcvsize)
		:m_ch(rt)
		, m_timer(ti)
		,m_pool(pool)
		,m_recvb(m_pool->get_block(max_rcvsize))
		,m_nid(nid)
		, m_last_active_tick(0)
		, m_timeo(0)
		,m_pdata(NULL)
	{
		m_recvb->wpos = 8;
		m_ch.set_listener(static_cast<tcpchannelListener*>(this));
	}

	tcppeer::~tcppeer()
	{
		m_ch.set_listener();
		m_recvb->release();
	}
	int tcppeer::disconnect()
	{
		m_recvb->rpos = 0;
		m_recvb->wpos = 8;
		if (DISCONNECTED == m_ch.get_state())
		{
			//未连接时也要回调，上层在Disconnected()时才删除
			call(tcppeerListener::Disconnected(), this);
			return 0;
		}
		return m_ch.disconnect();
	}
	void tcppeer::set_timeout(int tick)
	{
		m_timeo = tick;
		if (m_timer)
		{
			if(m_timeo>0)
				m_timer->register_timer(static_cast<timerHandler*>(this), 1, m_timeo);
			else
				m_timer->unregister_all(static_cast<timerHandler*>(this));
		}
	}
	void tcppeer::on(Connected, tcpchannel* ch) 
	{ 
		m_last_active_tick = mtick();
		call(tcppeerListener::Connected(), this); 
	}
	void tcppeer::on(Disconnected, tcpchannel* ch) 
	{
		if (m_timer && m_timeo>0)
			m_timer->unregister_all(static_cast<timerHandler*>(this));
		call(tcppeerListener::Disconnected(), this);
	}
	void tcppeer::on(Writable, tcpchannel* ch) 
	{ 
		m_last_active_tick = mtick();
		call(tcppeerListener::Writable(), this); 
	}
	void tcppeer::on(Readable, tcpchannel* ch, int* pwait)
	{
		m_last_active_tick = mtick();
		int cpsize = 0;
		while (CONNECTED == m_ch.get_state())
		{
			cpsize = ch->recv(m_recvb->read_ptr(), m_recvb->length());
			if (cpsize <= 0)
				return;
			m_recvb->rpos += cpsize;
			if (0 == m_recvb->length())
				on_data();
		}
	}
	void tcppeer::on_data()
	{
		if (8 == m_recvb->wpos)
		{
			uint32_t nid;
			int size;
			bstream b(m_recvb->buf, 8, 8);
			b >> nid;
			b >> size;
			if (m_nid != nid || size < 9 || size > m_recvb->buflen)
			{
				DEBUGMSG("#****wrong packet!****\n");
				disconnect(); //里面会删除m_recvb
				return;
			}
			m_recvb->wpos = size;
		}
		else
		{
			m_recvb->rpos = 0;
			call(tcppeerListener::Packet(), this, m_recvb->buf[8], m_recvb->buf + 9, m_recvb->wpos - 9);
			if (CONNECTED != m_ch.get_state())
				return;
			m_recvb->wpos = 8;
		}
	}
	void tcppeer::on_timer(int e)
	{
		//只处理半死连接,1分钟后无响应，则断开
		uint64_t tick = mtick();
		if (time_after(tick, m_last_active_tick + m_timeo))
			disconnect();
	}
}
