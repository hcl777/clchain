#pragma once
#include "udpac.h"
#include "p2pproto.h"
#include <vector>
#include "timer.h"
#include "bstream.h"
#include "speaker.h"

namespace p2p {

	class table;
	class tableListener
	{
	public:
		virtual ~tableListener(void) {}

		template<int I>
		struct S { enum { T = I }; };
		typedef S<1>	RspNode;

		virtual void on(RspNode,const node_t& n) {}

	};
	typedef struct tag_nodeinfo_t {
		node_t				node;
		time_t				begin_time = time(NULL); //second
		uint64_t			last_tick = mtick(); //最后活跃时间milli_second
		sockaddr_in			addr;
		tag_nodeinfo_t*		new_ni = NULL; //用于考虑新节点替代
		tag_nodeinfo_t() {  }

	}nodeinfo_t;

	class table : public udpacHandler
		,public cl::timerHandler
		,private cl::speaker<tableListener>
	{
	public:
		table(cl::timer *t);
		virtual ~table();
		typedef list<nodeinfo_t*> NodeList;
		typedef NodeList::iterator NodeIter;
	public:
		int init(const nodeid_t& strid, const netid_t& netid,uint16_t port);
		void fini();
		int update_nid(const nodeid_t& nid);
		int find_node(const nodeid_t& nid);

		virtual void on_timer(int e);
	protected:
		static NodeIter find_nodeiter(const nodeid_t& id, NodeList& ls);
		static nodeinfo_t* find_nodeinfo(const nodeid_t& id, NodeList& ls)
		{
			NodeIter it = find_nodeiter(id, ls);
			if (it != ls.end()) return *it;
			return NULL;
		}
		nodeinfo_t* find_nodeinfo(const nodeid_t& id){
			return find_nodeinfo(id, m_vec[hash_distance(id, m_my.node.id)]);
		}
		int add_node(const std::string& strid, const std::string& ipport);
		int remote_node(nodeid_t& id);

		//获取比dis更近的节点
		static void get_mindis_node(int dis,const nodeid_t& id, NodeList& ls, size_t maxsize, NodeList& srcls);
		//获取连头的节点，并且移到尾部
		static void get_loop_node(NodeList& ls,size_t maxsize, NodeList& srcls);
		void find_neighbours(const nodeid_t& id,NodeList& ls, size_t num);

		virtual int on_recv(char* buf, int size, sockaddr_in& addr);
		void try_add_node(const node_t& node); //尝试加入新节点


		template<typename T>
		int streamT(bstream& s,cmd_t cmd, netid_t netid, T& t)
		{
			pt_header_t h;
			h.netid = netid;
			h.cmd = cmd;
			h.seq = m_send_seq++;
			h.timestamp = (uint32_t)time(NULL);
			s << h;
			s << t;
			return s.ok();
		}
		template<typename T>
		int sendT(cmd_t cmd, T& t, sockaddr_in& addr)
		{
			bstream s(1024);
			streamT<T>(s, cmd, m_netid, t);
			return this->send(s.buffer(), s.length(), addr);
		}
		template<typename T>
		int sendT(cmd_t cmd, T& t, sockaddr_in& addr, netid_t netid)
		{
			bstream s(1024);
			streamT<T>(s, cmd, netid, t);
			return this->send(s.buffer(), s.length(), addr);
		}

	private:
		netid_t						m_netid;
		nodeinfo_t					m_my;//
		vector<NodeList>			m_vec;
		NodeList					m_pendigs;
		cl::timer					*_timer;
		int							m_find_neighbours_flag;
		uint32_t					m_send_seq;

	};

}
