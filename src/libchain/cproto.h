#pragma once
//#include "p2pproto.h"
#include "block.h"
#include "cpo.h"

//��ʶЭ��
/*
referee ����ʱ����һ���ȼ���˫�����һ��������Э�飬�����ǣ�
	�Լ�ID+�Է�ID+��������ʱ�䣬����ǩ�����Է��յ�ʱ���顣�ﵽһ�������ܼ���Է���ʵ��ݣ�
	��ʱ��У�ԡ��������������ִ��ݶ�̬���ݵ�Ч�����Ӷȣ�
*/

//using namespace p2p;
using cmd_t = uint8_t;
static const cmd_t	CONS_ = 0x40;
#define PTL_CONS(v,name) static const cmd_t CONS_##name = CONS_ | v

//�϶Ը߶�
PTL_CONS(0x01, REQ_HEIGHT); 
PTL_CONS(0x02, RSP_HEIGHT);
//��ȡ������
PTL_CONS(0x03, REQ_BLOCKS);
PTL_CONS(0x04, RSP_BLOCK);

//
PTL_CONS(0x05, HANDSHAKE);			//��Ա���֣���������ʱ��˫��ͬʱ���ּ��飬�첻��ֱ�Ӷϡ�
PTL_CONS(0x06, REQ_PROPOSER);		//��������Ȩ���㲥
PTL_CONS(0x07, RSP_PROPOSER);		//������Ȩ�ظ�
PTL_CONS(0x08, VERIFY_PROPOSER);	//�����������Ȩ���㲥
PTL_CONS(0x09, SYNC_PROPOSER);		//ͬ������״̬����3���������˻�ù�����Ȩ�ã����㲥
//
PTL_CONS(0x0A, REQ_BLOCKSIGN);			//����飬�㲥
PTL_CONS(0x0B, RSP_BLOCKSIGN);			//ǩ��ظ�
PTL_CONS(0x0C, PUB_BLOCKSIGN);			//�����飬�㲥

PTL_CONS(0x0D, BROADCAST_TRANS);			//�㲥����

enum CONS_ERROR {
	ECONS_TIME=1,
};


//req_synheight: null
//rsp_synheight: blocktag_t ���һ��

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
	int64_t				t; //���ʱ�䣬����У�ԣ�Ҳ��Ϊ��̬����
	hash_t				hash; //����
	string				sign; //ǩ��
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
//����㹻�𸴺󣬹㲥�ѻ�õ���Ȩ
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
	blocktag_t				bt; //��������Ϣ�������û�����������п���Ϣ�����ǩ���б�������û���������򷢲��ڵ�Ҫ���ݡ�
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

