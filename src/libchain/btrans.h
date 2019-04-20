#pragma once

#include "common.h"



enum TRANSACTION_TYPE {
	TRT_TRANSFER = 1,
	TRT_DIRECTOR,
	TRT_REFEREE,
	TRT_TOKEN,
	TRT_CONSTRACT
};

typedef struct tag_signature {
	userid_t		id;
	string			sign; //HASH的加密串
	tag_signature() {}
	tag_signature(const userid_t& _id, const string& _sign) :id(_id), sign(_sign) {}
	//bool operator==(const tag_signature& s)const { return (id == s.id&&sign == s.sign); }
}signature_t;

typedef struct tag_tran_out {
	userid_t			id; //自适应
	uint64_t			asset; //必须>0
}tran_out_t;

//
//typedef struct tag_tran_key {
//	userid_t			id;
//	string				pubkey;
//}tran_key_t;
//
typedef struct tag_tran_names {
	uint64_t			height = 0; //用于支持回滚
	vector<userid_t>	names;
	//uint64_t			timestamp = 0;
}tran_names_t;

//发行token的transaction_cert_t 里面的ID必须是发行者ID
typedef struct tag_tran_token {
	hash_t				hash; //临时
	uint32_t			cid;
	userid_t			id;
	uint64_t			assets; //发行总量
	string				name; //
	string				description; //
	string				pic; //数据为小图标
	string				pub_sign; //发行者
	signature_t			director_sign; //
}tran_token_t;

/*
外部产生一个交易时，得到一个交易流水号： tranhash#in_height
以后以此流水号查询交易是否成功：-1表失败，0表未知（块高度未达到0),1表成功
*/
//凭证certificate
typedef struct tag_transaction_cert {
	uint32_t			cid; //指明交易属于哪条链
	string				id; //
	string				tid;//tokenid,考虑支持禁用的情况。
	uint64_t			in_height = 0;	//插入指定范围有效，in_height~in_height+in_ex
	//uint64_t			as_height;//as_height虽检验效率最高，但同用户不能同时多笔交易插进不同块中。
	uint64_t			fee = 0; //手续费,地址类免
	int64_t				timestamp;
	string				ext; //用户扩展数据，例如交易备注
	uint8_t				type; //决定下列哪数据有效
	vector<tran_out_t>	outs;
	//tran_key_t			key;
	//vector<userid_t>	names;
	tran_names_t		ns; //height批向上一次变更的时候
	tran_token_t		tok;

	string to_asid()const { return id + tid; }
	string to_outid(const userid_t& u) const{return u+tid;}

}transaction_cert_t;

typedef struct tag_transaction {
	string				sign; //对后面数据的签名
	hash_t				tranhash; //临时
	transaction_cert_t	data;
}transaction_t;

//********************************************************
void hash_transaction(transaction_t& i);
void hash_trantoken(tran_token_t& i);

int operator << (bstream& s, const signature_t& i);
int operator >> (bstream& s, signature_t& i);

//int operator << (bstream& s, const tran_key_t& i);
//int operator >> (bstream& s, tran_key_t& i);

int operator << (bstream& s, const tran_out_t& i);
int operator >> (bstream& s, tran_out_t& i);

int tran_token_hashdata_stream(bstream& s, const tran_token_t& i);
int operator << (bstream& s, const tran_token_t& i);
int operator >> (bstream& s, tran_token_t& i);

int operator << (bstream& s, const transaction_cert_t& i);
int operator >> (bstream& s, transaction_cert_t& i);
int operator << (bstream& s, const transaction_t& i);
int operator >> (bstream& s, transaction_t& i);

//********************************************************
namespace trans
{
	int new_tran(transaction_t& t, uint32_t cid,ecuser_t& u, const string& tokid, uint64_t fee, uint64_t in_height, vector<tran_out_t>& outs,const string& ext);
	int new_names(transaction_t& t, uint32_t cid, ecuser_t& u, uint64_t in_height, tran_names_t& ns, uint8_t type);
	int new_token(transaction_t& t, uint32_t cid, ecuser_t& u, ecuser_t& dire, uint64_t in_height,
		uint64_t assets,const string& name,const string& description, uint64_t fee);
}
