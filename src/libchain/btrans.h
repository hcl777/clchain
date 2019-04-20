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
	string			sign; //HASH�ļ��ܴ�
	tag_signature() {}
	tag_signature(const userid_t& _id, const string& _sign) :id(_id), sign(_sign) {}
	//bool operator==(const tag_signature& s)const { return (id == s.id&&sign == s.sign); }
}signature_t;

typedef struct tag_tran_out {
	userid_t			id; //����Ӧ
	uint64_t			asset; //����>0
}tran_out_t;

//
//typedef struct tag_tran_key {
//	userid_t			id;
//	string				pubkey;
//}tran_key_t;
//
typedef struct tag_tran_names {
	uint64_t			height = 0; //����֧�ֻع�
	vector<userid_t>	names;
	//uint64_t			timestamp = 0;
}tran_names_t;

//����token��transaction_cert_t �����ID�����Ƿ�����ID
typedef struct tag_tran_token {
	hash_t				hash; //��ʱ
	uint32_t			cid;
	userid_t			id;
	uint64_t			assets; //��������
	string				name; //
	string				description; //
	string				pic; //����ΪСͼ��
	string				pub_sign; //������
	signature_t			director_sign; //
}tran_token_t;

/*
�ⲿ����һ������ʱ���õ�һ��������ˮ�ţ� tranhash#in_height
�Ժ��Դ���ˮ�Ų�ѯ�����Ƿ�ɹ���-1��ʧ�ܣ�0��δ֪����߶�δ�ﵽ0),1��ɹ�
*/
//ƾ֤certificate
typedef struct tag_transaction_cert {
	uint32_t			cid; //ָ����������������
	string				id; //
	string				tid;//tokenid,����֧�ֽ��õ������
	uint64_t			in_height = 0;	//����ָ����Χ��Ч��in_height~in_height+in_ex
	//uint64_t			as_height;//as_height�����Ч����ߣ���ͬ�û�����ͬʱ��ʽ��ײ����ͬ���С�
	uint64_t			fee = 0; //������,��ַ����
	int64_t				timestamp;
	string				ext; //�û���չ���ݣ����罻�ױ�ע
	uint8_t				type; //����������������Ч
	vector<tran_out_t>	outs;
	//tran_key_t			key;
	//vector<userid_t>	names;
	tran_names_t		ns; //height������һ�α����ʱ��
	tran_token_t		tok;

	string to_asid()const { return id + tid; }
	string to_outid(const userid_t& u) const{return u+tid;}

}transaction_cert_t;

typedef struct tag_transaction {
	string				sign; //�Ժ������ݵ�ǩ��
	hash_t				tranhash; //��ʱ
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
