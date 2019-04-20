#pragma once

#include "btrans.h"

//Electricity fee
/*
修改直接用ID做KEY更高效
单独增加一条辅链用于记录用户pubkey,如果用户需要，也可以记录用户的AES加密后的privatekey
这种情况下可以考虑主链去掉key交易功能（或者保留主链上也无所谓）

referee：
	名单变更策略暂时由director手动变更。
	后续可考虑如备用节点通过pow竞争排名，然后director自动签单变更。
*/


//控制最大不超过10M
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
	uint8_t			in_ex=1;//指定插入向后的块数，一般为1
	uint64_t		min_tranfee=0;//转账和部署合约手续费
	uint64_t		min_consfee=0;
}chain_setting_t;

/*
区块指针记录hash，但区块结构自己不记录hash，设计的hash属性只用于临时计算使用
*/
//typedef struct tag_block_point
//{
//	uint64_t		height;
//	hash_t			blockhash;
//}block_point_t;

typedef struct tag_assets
{
	uint64_t			assets=0; //单位为利，即1000个单位为1币
	uint64_t			height=0;
	uint8_t				flag=0; //todo:考虑用于冻结功能
}assets_t;

//user_assets_t 为用户总账,key为string id
typedef struct tag_user_assets
{
	string			id;
	assets_t		as; //bp指向最近
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

//注意：为了后面的扩展性，应该增加版本字段 uint8_t ver;
typedef struct tag_block {
	uint32_t				cid;
	uint8_t					ver = 0;
	uint64_t				height;
	hash_t					pre_hash;
	uint64_t				timestamp;
	vector<hash_t>			tranhashs; //用于快速查询交易确认，与后面的交易一一对应
	vector<user_assets_t>	uas; //UT,仅保存TRT_TRANSFER，其它链指针则直接指向具体交易
	tran_out_t				pub_out;//产量

	hash_t					blockhash;//临时，对前面数据+所有tranhash签名
	string					pub_sign; //发布者对cid至trannodes的block_hash的签名，发布者地址体现在首笔交易上。
	list<signature_t>		vote_signs;//投票签名列表
	vector<transaction_t>	trans; //首交易用户为发布者

	//交易流水，临时数据不传递，在check_block时生成
	map<string, flows_t>	mpf;
}block_t;

//用于保存相关索引信息，blockhash,tranhashs,方便查找确认交易是否已经写入
typedef struct tag_blocksummary
{
	hash_t					blockhash;
	vector<hash_t>			tranhashs;
}tag_blocksummary_t;


void hash_block_all(block_t& h);
void hash_block(block_t& b);

/*
统计所有变动账户存入uas
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


