#include "peersvr.h"
#include "common.h"
#include "cproto.h"
#include "chain.h"
#include "blocksvr.h"
#include "referee.h"

peersvr::peersvr()
{
}


peersvr::~peersvr()
{
}
int peersvr::init()
{
	uint16_t port = comSngl::instance()->na.local.port;
	m_tcpac.open(port, NULL, static_cast<tcpacceptorHandle*>(this), &comSngl::instance()->reactor);
	comSngl::instance()->timer.register_timer(static_cast<timerHandler*>(this), 1, 2000);
	return 0;
}
void peersvr::fini()
{
	m_tcpac.close();
	comSngl::instance()->timer.unregister_all(static_cast<timerHandler*>(this));
	while (!m_peers.empty())
		m_peers.front()->disconnect();
	pending_del();
}
void peersvr::pending_del()
{
	list<tcppeer*> ls;
	ls.swap(m_pending);
	for (list<tcppeer*>::iterator it = ls.begin(); it != ls.end(); ++it)
	{
		delete *it;
	}
}
tcppeer* peersvr::new_peer()
{
	tcppeer *peer = new tcppeer(&comSngl::instance()->reactor, &comSngl::instance()->timer,
		&comSngl::instance()->mempool, chainSngl::instance()->cid(),MB1M);
	peer->set_listener(this);
	m_peers.push_back(peer);
	return peer;
}
void peersvr::put_peer(tcppeer* peer)
{
	//find(first,last,val),找不到就=last,而不一定是end
	list<tcppeer*>::iterator it = find(m_peers.begin(), m_peers.end(), peer);
	if (it != m_peers.end())
	{
		peer->set_listener();
		m_peers.erase(it);
		peer->disconnect();
		m_pending.push_back(peer);
	}
	else
	{
		assert(0);
	}
}

void peersvr::on_timer(int e)
{
	switch (e)
	{
	case 1:
	{
		pending_del();
	}
	break;
	default:
		assert(0);
		break;
	}
}
bool peersvr::attach_tcp_socket(int fd, sockaddr_in& addr)
{
	tcppeer* peer = new_peer();
	if (NULL == peer)
		return false;
	if (0 != peer->attach(fd, addr))
	{
		put_peer(peer);
		return false;
	}
	peer->set_timeout(30000);
	return true;
}
void peersvr::on(tcppeerListener::Packet, tcppeer* peer, uint8_t cmd, char* buf, int len)
{
	switch (cmd)
	{
	case CONS_REQ_HEIGHT:
	{
		blocksvrSngl::instance()->rsp_height(peer);
		break;
	}
	case CONS_REQ_BLOCKS:
	{
		//attach 2 blocksvr
		blocksvrSngl::instance()->attach(peer);
		blocksvrSngl::instance()->on(tcppeerListener::Packet(), peer, cmd, buf, len);
		break;
	}
	case CONS_HANDSHAKE:
	{
		bstream s(buf, len, len);
		pc_handshake_t inf;
		if (0 != s >> inf)
			break;
		if (refereeSngl::instance()->attach_peer(peer, peer->channel()->get_ep(), inf.my))
			refereeSngl::instance()->on_handshake(peer, inf);
		else
			peer->disconnect();
		break;
	}
	default:
		assert(false);
		break;
	}
}

