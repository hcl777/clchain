#pragma once

#include "clhash.h"
#include "incnet.h"

namespace p2p {
	using namespace cl;
	using namespace std;

	using cmd_t = uint8_t;
	using nodeid_t = h256;
	using netid_t = uint32_t;
	static const cmd_t	P2P_ = 0x10;
#define PTL(v,name) static const cmd_t name = P2P_ | v

	PTL(0x01, NETWRONG);
	PTL(0x03, PING);
	PTL(0x04, PONG);
	PTL(0x05, FIND_NEIGHBOURS);
	PTL(0x06, RSP_NEIGHBOURS);
	PTL(0x07, FIND_NODE);
	PTL(0x08, RSP_NODE);


	typedef struct {
		netid_t		netid;
		uint8_t		cmd;
		uint32_t	seq; //包的唯一序列号
		uint32_t	timestamp;
	}pt_header_t;

	typedef struct tag_node{
		//以下ID使用二进制保存
		nodeid_t		id;//节点KAD网ID,也是应用账号ID，会随应用层变更而变更
		endpoint_t		pub; //外网
		endpoint_t		local; //内网,TCP只使用local中的port
		uint8_t			nat_type;

		bool operator ==(const tag_node& n)const { return n.id == id; }
	}node_t;


	//ping: node_t
	//pong: endpoint_t pub node_t

	//find_neighbours; nodeid_t,一次查找自己，一次随机，保证有向自己收，也有全网随机数据
	//rsp_neighbours: list node_t

	//find_referees: null
	//rsp_referees: 随机返回3个。list node_t 

	//find_node: 如果自己不是nat1类型的，t=1，即不跳转搜索，因为包不原路返回。
	typedef struct {
		uint8_t		t; //time of live,每转发一次减一
		nodeid_t	id;
		endpoint_t	from_pub;
		endpoint_t	from_local;
	}pt_find_node_t;
	//rsp_node: node_t
	
	void set_addr(sockaddr_in& addr,const endpoint_t& ep);

	int operator << (bstream& ss, const pt_header_t& i);
	int operator >> (bstream& ss, pt_header_t& i);

	int operator << (bstream& ss, const node_t& i);
	int operator >> (bstream& ss, node_t& i);

	int operator << (bstream& ss, const pt_find_node_t& i);
	int operator >> (bstream& ss, pt_find_node_t& i);
}


