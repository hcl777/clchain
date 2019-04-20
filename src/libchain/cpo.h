#pragma once

#include "block.h"
/*
key长度不能为8,否则与height重复
chain pointer:
0.chain_id: key="#chain_id"
1.last: key="#lastblockinfo"
2.addr: key=hash,val=pubkey
3.assets: key=hash+"A"
4.referees: key="#referees"
5.directors: key = "#directors"
	授权过程不放进链，发起者直接通过P2P网查找到任一成员提交申请，通过后有授权者部署到链。
6.token：指向授权者发布，key=hash+"T"
7.tokenkeys: 保存所有token key地址， key="#tokenkeys".
*/

//长度不能为8
#define DBK_CHAINSETTING		"#chainsetting"
#define DBK_LBI				"#lastblockinfo"
#define DBK_REFEREES		"#referees"
#define DBK_DIRECTORS		"#directors"
#define DBK_TOKENKEYS		"#tokenkeys"
#define DBK_TOKENPRE		"T"

//chain id : uint32(chain_id)

//last
typedef struct tag_blocktag
{
	uint64_t			height = 0;
	hash_t				blockhash;
	bool operator==(const tag_blocktag& b)const { return (b.height == height && b.blockhash == blockhash); }
	bool operator!=(const tag_blocktag& b)const { return (b.height != height || b.blockhash != blockhash); }
}blocktag_t;

typedef struct tag_cpo_last {
	blocktag_t			bt;
	uint64_t			timestamp=0;
}cpo_last_t;

//addr: string(pubkey)

//assets: assets_t(指向最新）

//referees: list<userid_t>

//directors: list<userid_t> //后面可考虑增加对每个userid_t的权限类型（uchar)permission

//token：block_point_t

int operator << (bstream& s, const blocktag_t& i);
int operator >> (bstream& s, blocktag_t& i);
int operator << (bstream& s, const cpo_last_t& i);
int operator >> (bstream& s, cpo_last_t& i);

int operator << (bstream& s, const tran_names_t& i);
int operator >> (bstream& s, tran_names_t& i);


