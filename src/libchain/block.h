#pragma once

#include "btrans.h"

//Electricity fee
/*
�޸�ֱ����ID��KEY����Ч
��������һ���������ڼ�¼�û�pubkey,����û���Ҫ��Ҳ���Լ�¼�û���AES���ܺ��privatekey
��������¿��Կ�������ȥ��key���׹��ܣ����߱���������Ҳ����ν��

referee��
	�������������ʱ��director�ֶ������
	�����ɿ����籸�ýڵ�ͨ��pow����������Ȼ��director�Զ�ǩ�������
*/


//������󲻳���10M
#define MAX_BLOCK_SIZE  1024*10240
#define BLOCK_OUTPUT	1000
#define INVALID_HEIGHT (uint64_t)0


enum BLOCK_ERROR {
	BERR_OK=0,
	BERR_ID,
	BERR_PRE,
	BERR_PUB_SIGN,
	BERR_VOTE_SIGN,
	BERR_TRAN,
	BERR_FEE,
	BERR_ASSETS,
	BERR_TRAN_HASH,
	BERR_TRAN_TIMESTAMP,
	BERR_TRAN_PUBOUT,
	BERR_DB_FAILD,
	BERR_DIRECTOR,
	BERR_REFEREE,
	BERR_TOKEN
};



typedef struct tag_chain_setting
{
	uint32_t		cid=0;
	uint8_t			in_ex=1;//ָ���������Ŀ�����һ��Ϊ1
	uint64_t		min_tranfee=0;//ת�˺Ͳ����Լ������
	uint64_t		min_consfee=0;
}chain_setting_t;

/*
����ָ���¼hash��������ṹ�Լ�����¼hash����Ƶ�hash����ֻ������ʱ����ʹ��
*/
//typedef struct tag_block_point
//{
//	uint64_t		height;
//	hash_t			blockhash;
//}block_point_t;

typedef struct tag_assets
{
	uint64_t			assets=0; //��λΪ������1000����λΪ1��
	uint64_t			height=0;
	uint8_t				flag=0; //todo:�������ڶ��Ṧ��
}assets_t;

//user_assets_t Ϊ�û�����,keyΪstring id
typedef struct tag_user_assets
{
	string			id;
	assets_t		as; //bpָ�����
	bool operator !=(const tag_user_assets& ua) const
	{
		if (id != ua.id) return true;
		if (as.assets != ua.as.assets) return true;
		if (as.height != ua.as.height) return true;
		return false;
	}
}user_assets_t;

enum FLOW_TYPE {
	FT_MINER_IN = 1,
	FT_FEE_OUT,
	FT_LC_IN,
	FT_LC_OUT,
	FT_TOK_IN,
	FT_TOK_OUT
};
typedef struct tag_flow_i
{
	int			type;
	string		desid;
	uint64_t	height;
	int			traidx;
	int64_t		asset;
	uint64_t	balance;
	uint64_t	t;
	tag_flow_i(int _type, const string& _did, uint64_t _h, int _traidx, uint64_t _asset, uint64_t _balance, uint64_t _t)
		:type(_type)
		, desid(_did)
		, height(_h)
		, traidx(_traidx)
		,asset(_asset)
		,balance(_balance)
		,t(_t)
	{
	}
}flow_i_t;
typedef struct tag_flows
{
	uint64_t		balance = 0;
	uint64_t		last_height = 0;
	list< flow_i_t>	ls;
}flows_t;

//ע�⣺Ϊ�˺������չ�ԣ�Ӧ�����Ӱ汾�ֶ� uint8_t ver;
typedef struct tag_block {
	uint32_t				cid;
	uint8_t					ver = 0;
	uint64_t				height;
	hash_t					pre_hash;
	uint64_t				timestamp;
	vector<hash_t>			tranhashs; //���ڿ��ٲ�ѯ����ȷ�ϣ������Ľ���һһ��Ӧ
	vector<user_assets_t>	uas; //UT,������TRT_TRANSFER��������ָ����ֱ��ָ����彻��
	tran_out_t				pub_out;//����

	hash_t					blockhash;//��ʱ����ǰ������+����tranhashǩ��
	string					pub_sign; //�����߶�cid��trannodes��block_hash��ǩ���������ߵ�ַ�������ױʽ����ϡ�
	list<signature_t>		vote_signs;//ͶƱǩ���б�
	vector<transaction_t>	trans; //�׽����û�Ϊ������

	//������ˮ����ʱ���ݲ����ݣ���check_blockʱ����
	map<string, flows_t>	mpf;
}block_t;

//���ڱ������������Ϣ��blockhash,tranhashs,�������ȷ�Ͻ����Ƿ��Ѿ�д��
typedef struct tag_blocksummary
{
	hash_t					blockhash;
	vector<hash_t>			tranhashs;
}tag_blocksummary_t;


void hash_block_all(block_t& h);
void hash_block(block_t& b);

/*
ͳ�����б䶯�˻�����uas
*/
void assets_map_add(map<string, int64_t>& mp, const string& id, int64_t v);
int make_block_assets(map<string, int64_t>& asmp, const tran_out_t& pub_out,const vector<transaction_t>& trans);

int operator << (bstream& s, const chain_setting_t& i);
int operator >> (bstream& s, chain_setting_t& i);

int operator << (bstream& s, const assets_t& i);
int operator >> (bstream& s, assets_t& i);

int operator << (bstream& s, const user_assets_t& i);
int operator >> (bstream& s, user_assets_t& i);

int block_hashdata_stream(bstream& s, const block_t& i);
int operator << (bstream& s, const block_t& i);
int operator >> (bstream& s, block_t& i);


