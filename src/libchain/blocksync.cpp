#include "blocksync.h"
#include "peersvr.h"
#include <algorithm>
#include "searcher.h"

#define MIN_ICONN 2

blocksync::blocksync()
	:m_iconn(0)
{
}


blocksync::~blocksync()
{
}
int blocksync::init()
{
	comSngl::instance()->timer.register_timer(static_cast<timerHandler*>(this), 1, 1000);
	comSngl::instance()->timer.register_timer(static_cast<timerHandler*>(this), 2, 5000);
	return 0;
}
void blocksync::fini()
{
	comSngl::instance()->timer.unregister_all(static_cast<timerHandler*>(this));
	while (!m_peers.empty())
		m_peers.begin()->second->disconnect();
	assert(0 == m_iconn);
}
void blocksync::on_timer(int e)
{
	switch (e)
	{
	case 1:
	{
		//检查更新height
		tcppeer* peer;
		pdata_t* pd;
		for (peeriter it = m_peers.begin(); it != m_peers.end(); ++it)
		{
			peer = it->second;
			pd = (pdata_t*)peer->get_pdata();
			if(PS_IDLE!=pd->state)
				peer->send_nullpacket(CONS_REQ_HEIGHT, MB1K);
		}
		break;
	}
	case 2:
	{
		//检查创建连接,非会员连接执行断开删除，因为如果其它节点也无连上会员，则都更新不到。
		tcppeer* peer;
		pdata_t* pd;
		nodeaddr_t na;
		list<tcppeer*> ls;
		map<userid_t, nodeaddr_t> mp;
		searcherSngl::instance()->get_referees(mp);
		if(!mp.empty())
		{
			//断开非会员
			for (peeriter it = m_peers.begin(); it != m_peers.end(); ++it)
			{
				if (mp.find(it->first) == mp.end())
					ls.push_back(it->second);
			}
			for (list<tcppeer*>::iterator it2 = ls.begin(); it2 != ls.end(); ++it2)
				(*it2)->disconnect();
			if (m_iconn<MIN_ICONN)
			{
				//清理map
				map<userid_t, nodeaddr_t>::iterator it3;
				for (it3 = mp.begin(); it3 != mp.end();)
				{
					//DEBUGMSG("id=%s  \n", b2h(it3->first).c_str());
					if (m_peers.find(it3->first) != m_peers.end() || comSngl::instance()->is_me(it3->first))
					{
						mp.erase(it3++);
					}
					else
						++it3;
				}
				//创建连接
				while (m_peers.size() < MIN_ICONN + 1 && !mp.empty())
				{
					get_rand_addr(na, mp);
					if (0 == na.pub.ip) continue;
					//DEBUGMSG("blocksync::connect(%s => %s)\n", b2h(na.id).c_str(), net::htoas(na.pub.ip,na.pub.port).c_str());
					
					peer = peersvrSngl::instance()->new_peer();
					peer->set_listener(static_cast<tcppeerListener*>(this));
					pd = new pdata_t();
					pd->id = na.id;
					pd->state = PS_IDLE;
					peer->set_pdata(pd);
					m_peers[na.id] = peer;
					if (0 != peer->connect(na.pub))
						on(tcppeerListener::Disconnected(), peer);
				}

			}
		}
		break;
	}
	default:
		assert(0);
		break;
	}
}
int blocksync::get_rand_addr(nodeaddr_t& n, map<userid_t, nodeaddr_t>& mp)
{
	//取完之后mp会删除
	if (mp.empty())
		return -1;
	srand((unsigned int)time(NULL));
	int num = rand()% mp.size();
	int i = 0;
	map<userid_t, nodeaddr_t>::iterator it = mp.begin();
	for (; i < num; ++i, ++it);
	n = it->second;
	mp.erase(it);
	return 0;
}
void blocksync::on(tcppeerListener::Connected, tcppeer* peer)
{
	pdata_t *pd = (pdata_t *)peer->get_pdata();
	pd->state = PS_CONN;
	m_iconn++;
	peer->send_nullpacket(CONS_REQ_HEIGHT, MB1K);
	DEBUGMSG("sync_on_conned(%s).\n", b2h(pd->id).c_str());
}
void blocksync::on(tcppeerListener::Disconnected, tcppeer* peer)
{
	pdata_t *pd = (pdata_t *)peer->get_pdata();
	if (pd->state != PS_IDLE)
	{
		m_iconn--;
		DEBUGMSG("sync_on_Disconned(%s).\n", b2h(pd->id).c_str());
	}
	for(list<uint64_t>::iterator it=pd->ts.begin();it!=pd->ts.end();++it)
		remove_task(peer, *it);
	m_peers.erase(pd->id);
	delete pd;
	peersvrSngl::instance()->put_peer(peer);
}
void blocksync::on(tcppeerListener::Packet, tcppeer* peer, uint8_t cmd, char* buf, int len)
{
	bstream s(buf, len, len);
	pdata_t *pd = (pdata_t *)peer->get_pdata();
	switch (cmd)
	{
	case CONS_RSP_HEIGHT:
	{
		blocktag_t bt;
		if (0 == s >> bt)
		{
			if (bt.height == INVALID_HEIGHT)
			{
				peer->disconnect();
				break;
			}
			pd->bt = bt;
			if (bt.height > m_bt.height)
			{
				m_bt = bt;
				update_task();
			}
			if (pd->ts.empty())
			{
				assign_job(peer,pd);
			}
		}
		break;
	}
	case CONS_RSP_BLOCK:
	{
		pc_rsp_block_t rsp;
		bool bok = false;
		if (0==s >> rsp)
		{
			if (0 == rsp.result)
			{
				if (0 == chainSngl::instance()->put_block((char*)rsp.s.data(), (int)rsp.s.size()))
				{
					remove_task(NULL, rsp.height);
					bok = true;
					DEBUGMSG("sync get(%llu) from(%s)\n", (unsigned long long)rsp.height,b2h(pd->id).c_str());
				}
			}
			if(!bok)
				remove_task(peer, rsp.height);
			pd->ts.remove(rsp.height);
			assign_job(peer, pd);
		}
		break;
	}
	default:
		assert(0);
		break;
	}
}
void blocksync::add_task(tcppeer* peer, list<uint64_t>& ls)
{
	list<uint64_t>::iterator it;
	for (it = ls.begin(); it != ls.end(); ++it)
	{
		m_ts[*it].ls.push_back(peer);
	}
}
void blocksync::remove_task(tcppeer* peer,uint64_t height)
{
	map<uint64_t, task_t>::iterator it = m_ts.find(height);
	if (it != m_ts.end())
	{
		if (peer)
			it->second.ls.remove(peer);
		else
			m_ts.erase(height);
	}
}
void blocksync::update_task()
{
	uint64_t i;
	list<uint64_t> ls;
	chainSngl::instance()->get_sync_task(m_bt.height, ls);
	for (list<uint64_t>::iterator it = ls.begin(); it != ls.end(); ++it)
	{
		i = *it;
		m_ts[i]; //直接执行分配
		//if (m_ts.find(i) == m_ts.end())
		//	m_ts[i] = task_t();
	}
}
void blocksync::assign_job(tcppeer* peer, pdata_t* pd)
{
	//在此更新task
	uint64_t n = chainSngl::instance()->get_last_height();
	if (m_ts.size() < 2 * MIN_ICONN && m_bt.height > n)
	{
		n = m_bt.height - n;
		if (n > m_ts.size())
			update_task();
	}

	//暂时同一时刻只配置1个任务
	int tn = 1 - (int)pd->ts.size();
	if (tn < 1)
		return;

	list<uint64_t> ls; //要保证ls中的任务不重复，并且不与现有任务重复
	

	//分配空闲任务
	for (map<uint64_t, task_t>::iterator it = m_ts.begin(); it != m_ts.end() && (int)ls.size()<tn; ++it)
	{
		n = it->first;
		if (it->second.ls.empty() && n <= pd->bt.height)
		{
			if (std::find(pd->ts.begin(), pd->ts.end(), n) == pd->ts.end())
			{
				ls.push_back(n);
			}
		}
	}
	//分配竞争任务时要检查 ls中是否已经存在
	//todo:
	
	//
	if (!ls.empty())
	{
		if (0 == peer->send_packet(CONS_REQ_BLOCKS, ls, MB1K))
		{
			pd->ts.insert(pd->ts.end(), ls.begin(),ls.end());
			add_task(peer, ls);
		}
	}
}
