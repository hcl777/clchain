#pragma once

#include "block.h"
/*
key���Ȳ���Ϊ8,������height�ظ�
chain pointer:
0.chain_id: key="#chain_id"
1.last: key="#lastblockinfo"
2.addr: key=hash,val=pubkey
3.assets: key=hash+"A"
4.referees: key="#referees"
5.directors: key = "#directors"
	��Ȩ���̲��Ž�����������ֱ��ͨ��P2P�����ҵ���һ��Ա�ύ���룬ͨ��������Ȩ�߲�������
6.token��ָ����Ȩ�߷�����key=hash+"T"
7.tokenkeys: ��������token key��ַ�� key="#tokenkeys".
*/

//���Ȳ���Ϊ8
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

//assets: assets_t(ָ�����£�

//referees: list<userid_t>

//directors: list<userid_t> //����ɿ������Ӷ�ÿ��userid_t��Ȩ�����ͣ�uchar)permission

//token��block_point_t

int operator << (bstream& s, const blocktag_t& i);
int operator >> (bstream& s, blocktag_t& i);
int operator << (bstream& s, const cpo_last_t& i);
int operator >> (bstream& s, cpo_last_t& i);

int operator << (bstream& s, const tran_names_t& i);
int operator >> (bstream& s, tran_names_t& i);


