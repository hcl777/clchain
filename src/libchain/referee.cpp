#include "referee.h"
#include "peersvr.h"
#include "searcher.h"
#include "signature.h"
#include "blockmaker.h"

#define TIMER_REFEREE_TICK 2000
referee::referee()
	: m_iconn(0)
{
}


referee::~referee()
{
}
int referee::init()
{
	chainSngl::instance()->add_listener(static_cast<chainListener*>(this));
	comSngl::instance()->timer.register_timer(static_cast<timerHandler*>(this),1,1000);
	comSngl::instance()->timer.register_timer(static_cast<timerHandler*>(this), 2, m_pp.get_timeout_ms()/20);
	update_referees();
	update_lbi();
	return 0;
}
void referee::fini()
{
	chainSngl::instance()->remove_listener(static_cast<chainListener*>(this));
	comSngl::instance()->timer.unregister_all(static_cast<timerHandler*>(this));
	free_all();
}
bool referee::attach_peer(tcppeer* peer, endpoint_t& ep, userid_t& id)
{
	peerinfo_t* pi;
	if (!comSngl::instance()->is_referee()) return false;
	if (comSngl::instance()->is_me(id)) return false;
	for (peeriter it = m_peers.begin(); it != m_peers.end(); ++it)
	{
		pi = *it;
		if (pi->n.id == id)
		{
			if (NULL != pi->peer)
			{
				if (cl::CONNECTED == pi->peer->channel()->get_state())
					return false;
				//用新的
				del_peer(pi);
			}
			
			pi->peer = peer;
			pi->peer->set_listener(static_cast<tcppeerListener*>(this));
			pi->peer->set_pdata(pi);
			pi->n.pub=ep;
			on(tcppeerListener::Connected(), peer);
			return true;
		}
	}
	return false;
}

void referee::on_timer(int e)
{
	//uint64_t tick = mtick();
	if (!comSngl::instance()->is_referee())
		return;
	switch (e)
	{
	case 1:
	{
		//将未连接的尝试建立连接
		if (m_iconn < (int)m_peers.size())
		{
			peerinfo_t* pi;
			for (peeriter it = m_peers.begin(); it != m_peers.end(); ++it)
			{
				pi = *it;
				if (DISCONNECTED != pi->peer->channel()->get_state())
					continue;
				if (0 == pi->n.pub.ip)
					searcherSngl::instance()->get_node(pi->n);
				if (0 != pi->n.pub.ip)
					pi->peer->connect(pi->n.pub);
			}
		}
		break;
	}
	case 2:
	{
		m_pp.on_timer_update_state();
		if (m_pp.on_timer_check_mypropose_begin())
		{
			//发起提议申请
			m_pp.begin_mypropose();
			ptl_req_propose();
		}
		else if (m_pp.is_mygiven())
		{
			std::string s;
			if (0 == blockmakerSngl::instance()->try_get_block(m_pp.m_myblock))
			{
				struct_to_string(m_pp.m_myblock, s, MAX_BLOCK_SIZE);
				{
					m_pp.set_state(proposer::BLOCKSIGN_ING);
					ptl_req_blocksign(s);
					//DEBUGMSG("makeblock ok: req_blocksign()...\n");
				}
			}
		}
		else if (m_pp.on_timer_check_sync_state())
		{
			//发起状态同步
			DEBUGMSG("%.2f: call_sync_state\n", mtick()/(double)1000);
			ptl_sync_propose();
			m_pp.reset_state();
		}
		break;
	}
	default:
		assert(0);
		break;
	}
}
void referee::on(chainListener::UpdateReferee)
{
	update_referees();
}
void referee::on(chainListener::UpdateLBI)
{
	update_lbi();
}

void referee::on(tcppeerListener::Connected, tcppeer* peer)
{
	peerinfo_t* pi = (peerinfo_t*)peer->get_pdata();
	pi->state = PI_CONN;
	m_iconn++;
	
	//handshake
	pc_handshake_t inf;
	inf.my = comSngl::instance()->id();
	inf.des = pi->n.id;
	inf.t = time(NULL);
	hash_handshake(inf);
	cryp::sig::sign(comSngl::instance()->prikey(), inf.hash, inf.sign);
	peer->send_packet(CONS_HANDSHAKE, inf, MB2K);
	//DEBUGMSG("ref::connect(%s) ok\n", b2h(pi->n.id).c_str());
}
void referee::on(tcppeerListener::Disconnected, tcppeer* peer)
{
	//这里一定有pdata
	peerinfo_t* pi = (peerinfo_t*)peer->get_pdata();
	if (pi->state != PI_IDLE)
		m_iconn--;
	pi->state = PI_IDLE;
	//DEBUGMSG("ref::disconnect(%s) \n", b2h(pi->n.id).c_str());
}
int referee::on_handshake(tcppeer* peer,pc_handshake_t& inf)
{
	peerinfo_t* pi = (peerinfo_t*)peer->get_pdata();
	hash_handshake(inf);
	//int64_t t = time(NULL);
	//时差对比意义不大
	if (!cryp::sig::verify(pi->n.id, inf.sign, inf.hash)/* || abs(t - inf.t) > 60*/)
	{
		//todo:
		pi->peer->disconnect();
		return -1;
	}
	pi->state = PI_READY;
	//DEBUGMSG("ref::on_handshake(%s) ok\n",b2h(pi->n.id).c_str());
	return 0;
}
void referee::on(tcppeerListener::Packet, tcppeer* peer, uint8_t cmd, char* buf, int len)
{
	peerinfo_t* pi = (peerinfo_t*)peer->get_pdata();
	bstream s(buf, len, len);
	switch (cmd)
	{
	case CONS_HANDSHAKE:
	{
		pc_handshake_t inf;
		if (0 != s >> inf)
			break;
		on_handshake(peer,inf);
		break;
	}
	case  CONS_REQ_PROPOSER:
	{
		//申请当提议者
		blocktag_t req;
		pc_rsp_propose_t rsp;
		if (0 != s >> req)
			break;
		m_pp.on_req_propose(pi->n.id,req, rsp);
		peer->send_packet(CONS_RSP_PROPOSER, rsp, MB2K);
		break;
	}
	case  CONS_RSP_PROPOSER:
	{
		pc_rsp_propose_t rsp;
		if (0 != s >> rsp)
			break;
		if (0 == rsp.result)
		{
			m_pp.add_vote(pi->n.id, rsp);
			if (m_pp.is_votes_ok())
			{
				DEBUGMSG("GIVEN.\n");
				m_pp.set_state(proposer::GIVEN);
				//开启block任务后，定时检查block是否生成
				blockmakerSngl::instance()->begin_make_block();
				ptl_verify_propose();
			}
		}
		break;
	}
	case  CONS_VERIFY_PROPOSER:
	{
		pc_verify_propose_t vp;
		if (0 == s >> vp)
			m_pp.on_verify_proposer(pi->n.id, vp);
		break;
	}
	case  CONS_SYNC_PROPOSER:
	{
		uint64_t	height;
		if (0 == s >> height)
			m_pp.on_sync_proposer(pi->n.id, height);
		break;
	}
	case  CONS_REQ_BLOCKSIGN:
	{
		cl::clslice sb;
		pc_rsp_blocksign_t rsp;
		if (0 == s >> sb)
		{
			m_pp.on_req_blocksign(pi->n.id, sb, rsp);
			peer->send_packet(CONS_RSP_BLOCKSIGN, rsp, MB2K);
			//DEBUGMSG("ON_req_blocksig:rsp.result=%d\n", rsp.result);
		}
		break;
	}
	case  CONS_RSP_BLOCKSIGN:
	{
		pc_rsp_blocksign_t rsp;
		if (0 == s >> rsp)
		{
			m_pp.handle_rsp_blocksign(pi->n.id, rsp);
			if (m_pp.is_blocksign_ok())
			{
				m_pp.m_myblock.vote_signs = m_pp.m_bsigns;
				//put block 会导致proposer 重置状态丢失签名，所以要先广播再添加
				ptl_pub_blocksign();
				if (0 == chainSngl::instance()->put_block(m_pp.m_myblock))
				{
					DEBUGMSG("new block(%llu)\n", (ull)rsp.height);
				}
			}
		}
		break;
	}
	case  CONS_PUB_BLOCKSIGN:
	{
		pc_pub_blocksign_t bs;
		if (0 == s >> bs)
		{
			
			if (bs.bt.blockhash == m_pp.m_last_give_block.blockhash && pi->n.id == m_pp.m_last_give_block.pub_out.id)
			{
				m_pp.m_last_give_block.vote_signs = bs.vote_signs;
				if (0 == chainSngl::instance()->put_block(m_pp.m_last_give_block))
				{
					DEBUGMSG("pub get(%llu)\n", (ull)bs.bt.height);
				}
			}
		}
		break;
	}
	case CONS_BROADCAST_TRANS:
	{
		transaction_t t;
		if (0 == s >> t)
		{
			blockmakerSngl::instance()->add_trans(t);
			DEBUGMSG("---------recv a broadcast trans--------\n");
		}
		break;
	}
	default:
		assert(false);
		break;
	}
}

void referee::update_referees()
{
	//取最新，并判断自己是否为会员
	//size_t i = 0;
	map<userid_t, int>	referees;
	map<userid_t, int>::iterator mit;
	chainSngl::instance()->get_referees(referees);

	if (!comSngl::instance()->is_referee())
	{
		free_all();
		
	}
	else
	{
		//断开非会员连接
		peerinfo_t* pi;
		peerlist ls;
		ls.swap(m_peers);

		peeriter it;
		for (it = ls.begin(); it != ls.end(); ++it)
		{
			pi = *it;
			mit = referees.find(pi->n.id);
			if (mit != referees.end())
				m_peers.push_back(pi);
			else
				put_peerinfo(pi);
		}
		for (mit = referees.begin(); mit != referees.end(); ++mit)
		{
			if (comSngl::instance()->is_me(mit->first) || find_pi(mit->first) != m_peers.end())
				continue;
			pi = new peerinfo_t();
			pi->n.id = mit->first;
			pi->peer = peersvrSngl::instance()->new_peer();
			pi->peer->set_listener(static_cast<tcppeerListener*>(this));
			pi->peer->set_pdata(pi);
			m_peers.push_back(pi);
		}
		
		m_pp.update_referees(referees);
		//此变更后会再调用块变更。
	}
	
}
void referee::update_lbi()
{
	m_pp.update_lbi();
}
void referee::free_all()
{
	for (peeriter it = m_peers.begin(); it != m_peers.end(); ++it)
	{
		put_peerinfo(*it);
	}
	m_peers.clear();
	assert(0 == m_iconn);
}
void referee::del_peer(peerinfo_t* pi)
{
	tcppeer* peer = pi->peer;
	peer->disconnect();
	pi->peer = NULL;
	peer->set_pdata(NULL);
	peersvrSngl::instance()->put_peer(peer);
}
referee::peeriter referee::find_pi(const userid_t& id)
{
	peeriter it;
	for (it = m_peers.begin(); it != m_peers.end(); ++it)
	{
		if((*it)->n.id==id) break;
	}
	return it;
}
void referee::ptl_req_propose()
{
	peeriter it;
	for (it = m_peers.begin(); it != m_peers.end(); ++it)
	{
		(*it)->peer->send_packet(CONS_REQ_PROPOSER, m_pp.m_lbi.bt, MB1K);
	}
}
void referee::ptl_verify_propose()
{
	pc_verify_propose_t vp;
	if (0 == m_pp.get_votes(vp))
	{
		for (peeriter it = m_peers.begin(); it != m_peers.end(); ++it)
		{
			(*it)->peer->send_packet(CONS_VERIFY_PROPOSER, vp, MB1K);
		}
	}
}
void referee::ptl_sync_propose()
{
	peeriter it;
	for (it = m_peers.begin(); it != m_peers.end(); ++it)
	{
		(*it)->peer->send_packet(CONS_SYNC_PROPOSER, m_pp.m_lbi.bt.height, MB1K);
	}
}
void referee::ptl_req_blocksign(const string& b)
{
	peeriter it;
	for (it = m_peers.begin(); it != m_peers.end(); ++it)
	{
		(*it)->peer->send_packet(CONS_REQ_BLOCKSIGN, b, MB1M);
	}
}
void referee::ptl_pub_blocksign()
{
	pc_pub_blocksign_t inf;
	if (0 == m_pp.get_bsigns(inf))
	{
		for (peeriter it = m_peers.begin(); it != m_peers.end(); ++it)
		{
			(*it)->peer->send_packet(CONS_PUB_BLOCKSIGN, inf, MB1K);
		}
	}
}
int referee::bloadcast_trans(transaction_t& t)
{
	peeriter it;
	for (it = m_peers.begin(); it != m_peers.end(); ++it)
	{
		(*it)->peer->send_packet(CONS_BROADCAST_TRANS, t, MB1M);
	}
	return 0;
}

