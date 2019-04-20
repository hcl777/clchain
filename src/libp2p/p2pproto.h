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
		uint32_t	seq; //����Ψһ���к�
		uint32_t	timestamp;
	}pt_header_t;

	typedef struct tag_node{
		//����IDʹ�ö����Ʊ���
		nodeid_t		id;//�ڵ�KAD��ID,Ҳ��Ӧ���˺�ID������Ӧ�ò��������
		endpoint_t		pub; //����
		endpoint_t		local; //����,TCPֻʹ��local�е�port
		uint8_t			nat_type;

		bool operator ==(const tag_node& n)const { return n.id == id; }
	}node_t;


	//ping: node_t
	//pong: endpoint_t pub node_t

	//find_neighbours; nodeid_t,һ�β����Լ���һ���������֤�����Լ��գ�Ҳ��ȫ���������
	//rsp_neighbours: list node_t

	//find_referees: null
	//rsp_referees: �������3����list node_t 

	//find_node: ����Լ�����nat1���͵ģ�t=1��������ת��������Ϊ����ԭ·���ء�
	typedef struct {
		uint8_t		t; //time of live,ÿת��һ�μ�һ
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


