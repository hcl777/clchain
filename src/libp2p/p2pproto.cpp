#include "p2pproto.h"

uint32_t g_netid = 0;

namespace p2p {

	void set_addr(sockaddr_in& addr, const endpoint_t& ep)
	{
		memset(&addr, 0, sizeof(sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ep.ip);
		addr.sin_port = htons(ep.port);
	}
	int operator << (bstream& ss, const pt_header_t& i)
	{
		ss << i.netid;
		ss << i.cmd;
		ss << i.seq;
		ss << i.timestamp;
		return ss.ok();
	}
	int operator >> (bstream& ss, pt_header_t& i)
	{
		ss >> i.netid;
		ss >> i.cmd;
		ss >> i.seq;
		ss >> i.timestamp;
		return ss.ok();
	}

	int operator << (bstream& ss, const node_t& i)
	{
		ss << i.id;
		ss << i.pub;
		ss << i.local;
		ss << i.nat_type;
		return ss.ok();
	}
	int operator >> (bstream& ss, node_t& i)
	{
		ss >> i.id;
		ss >> i.pub;
		ss >> i.local;
		ss >> i.nat_type;
		return ss.ok();
	}

	int operator << (bstream& ss, const pt_find_node_t& i)
	{
		ss << i.t;
		ss << i.id;
		ss << i.from_pub;
		ss << i.from_local;
		return ss.ok();
	}
	int operator >> (bstream& ss, pt_find_node_t& i)
	{
		ss >> i.t;
		ss >> i.id;
		ss >> i.from_pub;
		ss >> i.from_local;
		return ss.ok();
	}
}


