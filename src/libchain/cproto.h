#pragma once
//#include "p2pproto.h"
#include "block.h"
#include "cpo.h"

//共识协议
/*
referee 互连时，第一步先检验双方身份一次性握手协议，方法是：
	自己ID+对方ID+最新日期时间，进行签名。对方收到时检验。达到一个包就能检验对方真实身份，
	和时间校对。（避免往返握手传递动态数据低效增复杂度）
*/

//using namespace p2p;
using cmd_t = uint8_t;
static const cmd_t	CONS_ = 0x40;
#define PTL_CONS(v,name) static const cmd_t CONS_##name = CONS_ | v

//较对高度
PTL_CONS(0x01, REQ_HEIGHT); 
PTL_CONS(0x02, RSP_HEIGHT);
//获取块数据
PTL_CONS(0x03, REQ_BLOCKS);
PTL_CONS(0x04, RSP_BLOCK);

//
PTL_CONS(0x05, HANDSHAKE);			//议员握手，刚连接上时的双向同时握手检验，检不过直接断。
PTL_CONS(0x06, REQ_PROPOSER);		//申请提议权，广播
PTL_CONS(0x07, RSP_PROPOSER);		//授提议权回复
PTL_CONS(0x08, VERIFY_PROPOSER);	//发布获得提议权，广播
PTL_CONS(0x09, SYNC_PROPOSER);		//同步提议状态（超3个周期无人获得过提议权用），广播
//
PTL_CONS(0x0A, REQ_BLOCKSIGN);			//提议块，广播
PTL_CONS(0x0B, RSP_BLOCKSIGN);			//签块回复
PTL_CONS(0x0C, PUB_BLOCKSIGN);			//发布块，广播

PTL_CONS(0x0D, BROADCAST_TRANS);			//广播交易

enum CONS_ERROR {
	ECONS_TIME=1,
};


//req_synheight: null
//rsp_synheight: blocktag_t 最后一块

//req_blocks: list
//rsp_block: int,blockbuffer
typedef struct tag_pc_rsp_block
{
	uint64_t			height;
	int					result;
	cl::clslice				s;
}pc_rsp_block_t;

typedef struct tag_pc_handshake
{
	userid_t			my;
	userid_t			des;
	int64_t				t; //最近时间，用于校对，也作为动态数据
	hash_t				hash; //不传
	string				sign; //签名
}pc_handshake_t;

//req_propose blocktag_t

typedef struct tag_pc_rsp_propose
{
	int32_t				result;
	int64_t				timestamp;
	string				sign; //signature(id+timestamp+blocktag_t)
}pc_rsp_propose_t;

typedef struct tag_pc_proposets
{
	userid_t			id;
	int64_t				timestamp;
	string				sign; 
}pc_proposets_t;
//获得足够答复后，广播已获得的授权
typedef struct tag_pc_verify_propose
{
	uint64_t				height;
	vector<pc_proposets_t>	signs;
}pc_verify_propose_t;

//pc_req_blocksign_t clslice
typedef struct tag_pc_rsp_blocksign
{
	int32_t				result;
	uint64_t			height;
	string				sign;
}pc_rsp_blocksign_t;

typedef struct tag_pc_pub_blocksign
{
	blocktag_t				bt; //发布块信息，其它用户如果手上已有块信息则加上签名列表即完整，没有再主动向发布节点要数据。
	list<signature_t>		vote_signs;
}pc_pub_blocksign_t;



int operator << (bstream& s, const pc_rsp_block_t& i);
int operator >> (bstream& s, pc_rsp_block_t& i);

void hash_handshake(pc_handshake_t& i);
int operator << (bstream& s, const pc_handshake_t& i);
int operator >> (bstream& s, pc_handshake_t& i);

int operator << (bstream& s, const pc_proposets_t& i);
int operator >> (bstream& s, pc_proposets_t& i);

int operator << (bstream& s, const pc_rsp_propose_t& i);
int operator >> (bstream& s, pc_rsp_propose_t& i);

int operator << (bstream& s, const pc_verify_propose_t& i);
int operator >> (bstream& s, pc_verify_propose_t& i);

int operator << (bstream& s, const pc_rsp_blocksign_t& i);
int operator >> (bstream& s, pc_rsp_blocksign_t& i);

int operator << (bstream& s, const pc_pub_blocksign_t& i);
int operator >> (bstream& s, pc_pub_blocksign_t& i);

