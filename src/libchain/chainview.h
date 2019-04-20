#pragma once
#include "chain.h"


int chain_view(const string& dbpath);


template<typename T>
bool db_readv(T& v, const string& key, leveldb::DB* db)
{
	string val;
	return (db->Get(leveldb::ReadOptions(), key, &val).ok() && 0==string_to_struct(val, v));
}

string& json_str(string& s,const chain_setting_t& i);
string& json_str(string& s, const cpo_last_t& i);
string& json_str(string& s, const tran_names_t& i);
string& json_str(string& s, const list<string>& i);
string& json_str(string& s, const tran_token_t& i);
string& json_str(string& s, const assets_t& i);
string& json_str(string& s, const block_t& i);

string& trancert_to_str(string& s,const transaction_cert_t& t);