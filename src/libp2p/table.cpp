#include "table.h"
#include "host.h"
#include "net.h"
#include "util.h"
//linux下使用std::find()要algorithm
#include <algorithm>

namespace p2p {

#define K_BUCKET_SIZE 16
#define VEC_SIZE 257

	using namespace cl;

	enum{
		TIMER_NEIGHBOURS=1,
		TIMER_NPLS
	};
	table::table(cl::timer *t)
		:speaker<tableListener>(1)
		,m_netid(0)
		,_timer(t)
		, m_find_neighbours_flag(0)
		, m_send_seq(0)
	{
	}


	table::~table()
	{
	}
	int table::init(const nodeid_t& strid, const netid_t& netid, uint16_t port)
	{
		m_my.node.id = strid;
		m_my.node.nat_type = 1;
		m_my.node.local.set(net::atoh(net::get_local_private_ip()), port);
		m_netid = netid;
		m_vec.resize(VEC_SIZE);

		//添加接入点
		//add_node(nodeid_t::rand_hash().to_hexString(), "192.168.5.244:20550");

		_timer->register_timer(static_cast<cl::timerHandler*>(this), TIMER_NEIGHBOURS, 8000);
		_timer->register_timer(static_cast<cl::timerHandler*>(this), TIMER_NPLS, 1000);
		return 0;
	}
	void table::fini()
	{
		_timer->unregister_all(static_cast<cl::timerHandler*>(this));
		for (int i = 0; i < VEC_SIZE; ++i)
		{
			NodeList& ls = m_vec[i];
			for (NodeIter it = ls.begin(); it != ls.end(); ++it)
			{
				if((*it)->new_ni)
					delete (*it)->new_ni;
				delete *it;
			}
			ls.clear();
		}
	}
	int table::update_nid(const nodeid_t& nid)
	{
		if (nid == m_my.node.id)
			return 0;
		m_my.node.id = nid;
		vector<NodeList> vec;
		NodeIter it;
		vec.swap(m_vec);
		m_vec.resize(VEC_SIZE);
		for (int i = 0; i < VEC_SIZE; ++i)
		{
			NodeList& ls = vec[i];
			for (it = ls.begin(); it != ls.end(); ++it)
			{
				m_vec[hash_distance((*it)->node.id, nid)].push_back(*it);
			}
		}
		return 0;
	}

	int table::find_node(const nodeid_t& nid)
	{
		pt_find_node_t fn;
		fn.id = nid;
		fn.t = 1;
		if (m_my.node.nat_type < 2)
			fn.t = 5;
		fn.from_local = m_my.node.local;
		fn.from_pub = m_my.node.pub;
	
		int dis = hash_distance(m_my.node.id, fn.id);
		nodeinfo_t* ni = find_nodeinfo(fn.id, m_vec[dis]);
		if (ni)
		{
			fire(tableListener::RspNode(), ni->node);
		}
		else
		{
			NodeList ls;
			find_neighbours(fn.id, ls, 3); //可能找到当前发过来的节点，暂不我这不是
			bstream s(1024);
			streamT(s, FIND_NODE, m_netid, fn);
			for (NodeIter it = ls.begin(); it != ls.end(); ++it)
			{
				this->send(s.buffer(), s.length(), (*it)->addr);
			}
		}
		return 0;
	}

	void table::on_timer(int e)
	{
		uint64_t tick = mtick();
		switch (e)
		{
		case TIMER_NEIGHBOURS:
		{
			NodeList ls;
			nodeid_t id;
			if (m_find_neighbours_flag == 0)
			{
				//找随机
				m_find_neighbours_flag = 1;
				id.rand();
			}
			else
			{
				//找自己
				m_find_neighbours_flag = 0;
				id = m_my.node.id;
			}
			find_neighbours(id,ls,3);

			nodeinfo_t* node;
			bstream s(1024);
			streamT(s, FIND_NEIGHBOURS, m_netid, id);
			for (NodeIter it = ls.begin(); it != ls.end(); ++it)
			{
				node = *it;
				this->send(s.buffer(), s.length(), node->addr);
			}
		}
			break;
		case TIMER_NPLS:
		{
			nodeinfo_t *ni, *tmp;
			while (!m_pendigs.empty())
			{
				ni = m_pendigs.front();
				if (!ni->new_ni)
				{
					m_pendigs.pop_front();
				}
				else
				{
					if (time_after(tick, ni->last_tick + 1600))
					{
						//加入新节点,不要删除原节点内存
						tmp = ni->new_ni;
						*ni = *tmp;
						ni->new_ni = NULL;
						memcpy(&ni->addr, &tmp->addr, sizeof(sockaddr_in));
						delete tmp;
						m_pendigs.pop_front();
					}
					else
					{
						break;
					}
				}
			}
		}
			break;
		default:
			assert(0);
			break;
		}
	}
	table::NodeIter table::find_nodeiter(const nodeid_t& id, NodeList& ls)
	{
		for (NodeIter it = ls.begin(); it != ls.end(); ++it)
		{
			if ((*it)->node.id == id)
				return it;
		}
		return ls.end();
	}

	int table::add_node(const std::string& strid, const std::string& ipport)
	{
		nodeid_t id = strid;
		int distance = hash_distance(m_my.node.id, id);
		if (0 == distance)
			return 2;
		if (find_nodeinfo(id, m_vec[distance]))
			return 1;

		nodeinfo_t *i = new nodeinfo_t();
		i->last_tick = cl::mtick();
		i->node.id = id;
		i->node.nat_type = 1;
		i->node.pub.set(net::atoh(get_string_index(ipport,0,":")),(uint16_t)cl::atoi(get_string_index(ipport, 1, ":"),0));
		set_addr(i->addr, i->node.pub);
		
		m_vec[distance].push_back(i);
		return 0;
	}
	int table::remote_node(nodeid_t& id)
	{
		NodeList& ls = m_vec[hash_distance(id, m_my.node.id)];
		NodeIter it;
		//先删除pending
		for (it = m_pendigs.begin(); it != m_pendigs.end(); ++it)
		{
			if ((*it)->node.id == id)
			{
				m_pendigs.erase(it);
				break;
			}
		}
		for (it = ls.begin(); it != ls.end(); ++it)
		{
			if ((*it)->node.id == id)
			{
				if ((*it)->new_ni) delete (*it)->new_ni;
				delete (*it);
				ls.erase(it);
				break;
			}
		}
		return 0;
	}
	void table::get_mindis_node(int dis, const nodeid_t& id, NodeList& ls, size_t maxsize, NodeList& srcls)
	{
		//size_t j = 0;
		int n;
		NodeIter it;
		NodeList tmpls;
		for (it = srcls.begin(); it != srcls.end();)
		{
			n = hash_distance(id, (*it)->node.id);
			if (n < dis)
			{
				ls.push_back(*it);
				tmpls.push_back(*it);
				srcls.erase(it++);
				if (ls.size() >= maxsize)
					break;
			}
			else
				++it;
		}
		if (ls.size() < maxsize)
		{
			for (it = srcls.begin(); it != srcls.end();)
			{
				ls.push_back(*it);
				tmpls.push_back(*it);
				srcls.erase(it++);
				if (ls.size() >= maxsize)
					break;
			}
		}
		for (it = tmpls.begin(); it != tmpls.end();++it)
		{
			srcls.push_back(*it);
		}
	}
	void table::get_loop_node(NodeList& ls, size_t maxsize, NodeList& srcls)
	{
		size_t j = 0;
		while (j < srcls.size())
		{
			j++;
			nodeinfo_t* node = srcls.front();
			srcls.pop_front();
			srcls.push_back(node);
			ls.push_back(node);
			if (ls.size() >= maxsize)
				return;
		}
	}
	void table::find_neighbours(const nodeid_t& id,NodeList& ls, size_t num)
	{
		//每次找3个点来查。不重发，不管是否成功，可能没联网
		int i;
		int dis = hash_distance(id,m_my.node.id);
		get_mindis_node(dis,id,ls, num,m_vec[dis]);
		if (ls.size() < num)
		{
			for (i = dis - 1; i > 0; i--)
			{
				get_loop_node(ls, num, m_vec[i]);
				if (ls.size() >= num)
					break;
			}
		}
		if (ls.size() < num)
		{
			for (i = dis + 1; i < VEC_SIZE; ++i)
			{
				get_loop_node(ls, num, m_vec[i]);
				if (ls.size() >= num)
					break;
			}
		}
	}
	int table::on_recv(char* buf, int size, sockaddr_in& addr)
	{
		pt_header_t h;
		bstream s(buf, size, size);
		if (0 != s >> h)
			return 0;
		if (h.netid != m_netid)
		{
			sendT(NETWRONG, m_my.node.id, addr, h.netid);
			return 0;
		}
		//暂时忽略时差问题
		//uint32_t t = (uint32_t)time(NULL);
		//if (time_distance(t, h.timestamp) > 3)
		//	return 0;

		switch (h.cmd)
		{
		case NETWRONG:
		{
			nodeid_t id;
			if (0 == s >> id)
				remote_node(id);
		}
			break;
		case PING:
		{
			node_t node,node2;
			if (0 != s >> node)
				return 0;
			node.pub.set((uint32_t)ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
			node2 = m_my.node;
			node2.pub = node.pub; //回复对方的pub
			sendT(PONG, m_my.node, addr);
			try_add_node(node);
		}
			break;
		case PONG:
		{
			node_t node;
			if (0 != s >> node)
				return 0;
			int dis = hash_distance(node.id, m_my.node.id);
			NodeList& ls = m_vec[dis];
			NodeIter it = find_nodeiter(node.id, ls);
			if (it != ls.end())
			{
				nodeinfo_t *ni = *it;
				memcpy(&ni->addr, &addr, sizeof(addr));
				ni->last_tick = mtick();
				ni->node.nat_type = node.nat_type;
				ni->node.local = node.local;
				ni->node.pub.set((uint32_t)ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
				if (ni->new_ni)
				{
					m_pendigs.remove(ni);
					//放弃新节点
					delete ni->new_ni; 
					ni->new_ni = NULL;
					//移到队尾
					ls.erase(it);
					ls.push_back(ni);
				}
				//对方返回自己的pub
				if (0== m_my.node.pub.ip && 0 != node.pub.ip 
					&& !net::is_private_ip(node.pub.ip))
					m_my.node.pub = node.pub;
			}
		}
			break;
		case FIND_NEIGHBOURS:
		{
			nodeid_t id;
			NodeList ls;
			list<node_t> ls2;
			if (0 == s >> id)
			{
				find_neighbours(id, ls, 3);
				for (NodeIter it = ls.begin(); it != ls.end(); ++it)
					ls2.push_back((*it)->node);
				sendT(RSP_NEIGHBOURS, ls2, addr);
			}
		}
			break;
		case RSP_NEIGHBOURS:
		{
			list<node_t> ls;
			if (0 == s >> ls)
			{
				for (list<node_t>::iterator it = ls.begin(); it != ls.end(); ++it)
				{
					try_add_node(*it);
				}
			}
		}
			break;
		case FIND_NODE:
		{
			pt_find_node_t fn;
			if (0 == s >> fn)
			{
				fn.t--; //查找不到，并且非0，则找近的3节点转发
				int dis = hash_distance(m_my.node.id, fn.id);
				nodeinfo_t* ni = find_nodeinfo(fn.id, m_vec[dis]);
				if (ni)
				{
					sockaddr_in toaddr1,toaddr2;
					set_addr(toaddr1, fn.from_pub);
					set_addr(toaddr2, fn.from_local);
					sendT(RSP_NODE, ni->node, toaddr1);
					sendT(RSP_NODE, ni->node, toaddr2);
				}
				else
				{
					if (fn.t > 0)
					{
						NodeList ls;
						find_neighbours(fn.id, ls, 3); //可能找到当前发过来的节点，暂不我这不是
						bstream s(1024);
						streamT(s, FIND_NODE, m_netid, fn);
						for (NodeIter it = ls.begin(); it != ls.end(); ++it)
						{
							this->send(s.buffer(), s.length(), (*it)->addr);
						}
					}
				}
			}
		}
			break;
		case RSP_NODE:
		{
			node_t n;
			if (0 == s >> n)
			{
				fire(tableListener::RspNode(), n);
			}
		}
			break;
		default:
			assert(0);
			break;
		}
		return 0;
	}
	void table::try_add_node(const node_t& node)
	{
		NodeIter it;
		int dis = hash_distance(node.id, m_my.node.id);
		NodeList& ls = m_vec[dis];

		//1.检查是否存在于vec or pending
		nodeinfo_t *ni = find_nodeinfo(node.id, ls);
		if (NULL != ni)
			return;
		for (it = m_pendigs.begin(); it != m_pendigs.end(); ++it)
		{
			if ((*it)->node.id == node.id)
			{
				return;
			}
		}

		nodeinfo_t *new_ni = new nodeinfo_t();
		new_ni->node = node;
		set_addr(new_ni->addr, new_ni->node.pub);
		if (ls.size() < K_BUCKET_SIZE)
		{
			ls.push_back(ni);
		}
		else
		{
			for (NodeIter it = ls.begin(); it != ls.end(); ++it)
			{
				ni = *it;
				if (ni->new_ni == NULL)
				{
					ni->new_ni = new_ni;
					m_pendigs.push_back(ni);
					sendT(PING, m_my.node, ni->addr); //如果有回复就用旧的
					break;
				}
			}
			
		}
	}
}
