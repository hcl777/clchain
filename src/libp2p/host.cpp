#include "host.h"


namespace p2p {
	host::host()
		:m_tb(&m_timer)
	{
	}


	host::~host()
	{
	}
	int host::init(uint16_t port, const nodeid_t& nid, const netid_t& netid, bool bthread)
	{
		int ret;
		m_tb.init(nid,netid,port);
		ret = m_ac.open(port, static_cast<udpacHandler*>(&m_tb));
		if (bthread)
			activate(1);
		return ret;
	}
	void host::fini()
	{
		m_brun = false;
		wait();
		m_ac.close();
		m_tb.fini();
	}
	
	void host::loop()
	{
		uint32_t delay_usec = 0;

		m_timer.handle_root();

		delay_usec = (uint32_t)m_timer.get_remain_us();
		delay_usec = delay_usec / 2;
		if (delay_usec>10000) delay_usec = 10000;

		m_ac.read_loop(delay_usec);
	}
	void host::work(int i)
	{
		while (m_brun)
			loop();
	}
}
