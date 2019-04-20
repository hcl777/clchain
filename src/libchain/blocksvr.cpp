#include "blocksvr.h"
#include "peersvr.h"
#include "chain.h"

blocksvr::blocksvr()
{
}


blocksvr::~blocksvr()
{
}
int blocksvr::init()
{
	return 0;
}
void blocksvr::fini()
{
	while (!m_peers.empty())
		m_peers.front()->disconnect();
}
bool blocksvr::attach(tcppeer* peer)
{
	peer->set_listener(static_cast<tcppeerListener*>(this));
	pdata_t *data = new pdata_t();
	peer->set_pdata(data);
	return true;
}

void blocksvr::on(Disconnected, tcppeer* peer)
{
	delete (pdata_t*)peer->get_pdata();
	peer->set_listener();
	m_peers.remove(peer);
	peersvrSngl::instance()->put_peer(peer);
}
void blocksvr::on(tcppeerListener::Writable, tcppeer* peer)
{
	pdata_t* data = (pdata_t*)peer->get_pdata();
	string s;
	pc_rsp_block_t rsp;
	while (!data->heights.empty())
	{
		rsp.height = data->heights.front();
		data->heights.pop_front();
		if (chainSngl::instance()->get_block(rsp.height, s))
		{
			rsp.result = 0;
			rsp.s.set(s.data(), s.size());
		}
		else
		{
			rsp.result = -1;
			rsp.s.clear();
		}
		DEBUGMSG("blocksvr::rsp_block(%llu) ret=%d\n", (ull)rsp.height, rsp.result);
		if (0 != peer->send_packet(CONS_RSP_BLOCK, rsp, MB1M))
			break;
	}
}
int blocksvr::rsp_height(tcppeer* peer)
{
	cpo_last_t lbi;
	chainSngl::instance()->get_last_blockinfo(lbi);
	return peer->send_packet(CONS_RSP_HEIGHT, lbi.bt, MB1K);
}

void blocksvr::on(tcppeerListener::Packet, tcppeer* peer, uint8_t cmd, char* buf, int len)
{
	//只处理2条协议
	bstream s(buf, len, len);
	pdata_t* data = (pdata_t*)peer->get_pdata();
	switch (cmd)
	{
	case CONS_REQ_HEIGHT:
	{
		rsp_height(peer);
		break;
	}
	case CONS_REQ_BLOCKS:
	{
		list<uint64_t> ls;
		if (0 != s >> ls)
			break;

		data->heights.insert(data->heights.end(), ls.begin(),ls.end());
		on(tcppeerListener::Writable(), peer);
		break;
	}
	default:
		assert(false);
		break;
	}
	assert(0 == s.length());
}
