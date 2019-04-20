#pragma once
#include "udpac.h"
#include <string>
#include "p2pproto.h"
#include "udpac.h"
#include "table.h"

#include "nat.h"
#include "stun.h"
#include "singleton.h"
#include "timer.h"
#include "worker.h"

namespace p2p {

	using namespace std;
	class host : public cl::worker
	{
	public:
		host();
		~host();
	public:
		int init(uint16_t port, const nodeid_t& nid, const netid_t& netid,bool bthread);
		void fini();
		void loop();
		table* tb() { return &m_tb; }

	protected:
		virtual void work(int i);

	private:
		cl::timer		m_timer;
		udpac			m_ac;
		table			m_tb;
	};
	typedef cl::singleton<host> hostSngl;
}


