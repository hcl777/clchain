#pragma once
#include "cproto.h"

/*
track rpc�� streamͨ�ø�ʽ
base64(data);
��Ҫ����ͷ����signature_t ͷ���
*/

using namespace std;
using namespace cl;


typedef struct tag_genesis_conf
{
	uint32_t	cid; //chain id
	string		genesisid; //��ʼ�û�
	uint64_t	timestamp;
}genesis_conf_t;

typedef struct tag_tp_state
{
	nodeaddr_t		na;
	cpo_last_t		lbi;
	uint64_t		timestamp=0;
}tp_state_t;

typedef struct tag_track_state
{
	uint64_t					begintime;
	genesis_conf_t				gc;
	map<userid_t, tp_state_t>	dirs;
	map<userid_t, tp_state_t>	refs;
}track_state_t;

//��ͷ������һ��signature_t
int tp_sign(const string& prikey, const string& pubkey, const string& data, string& out);
//���ǩ���Ƿ�OK=0
int tp_unsign(const string& s, cl::clslice& out_data, signature_t& sig);

int operator << (bstream& s, const genesis_conf_t& i);
int operator >> (bstream& s, genesis_conf_t& i);

int operator << (bstream& s, const tp_state_t& i);
int operator >> (bstream& s, tp_state_t& i);



void  xml_track_state(char* buf, track_state_t& ts);

