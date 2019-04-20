#include "chainview.h"
#include "util.h"
#include <iostream>
#include <stdio.h>

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

void view_all(leveldb::DB *db);
void view_set(leveldb::DB *db);
void view_lbi(leveldb::DB *db);
void view_directors(leveldb::DB *db);
void view_referees(leveldb::DB *db);
void view_tokenkeys(leveldb::DB *db);
void view_token(leveldb::DB *db);
void view_user(leveldb::DB *db);
void view_block(leveldb::DB *db);


int chain_view(const string& dbpath)
{
	int n;
	string s;
	leveldb::DB *db;
	leveldb::Options options;
	leveldb::Status st;
	//options.error_if_exists = true;
	//options.create_if_missing = true; 
	st = leveldb::DB::Open(options, dbpath, &db);
	if (!st.ok())
	{
		cout << "*** open db fail.\n";
		return -1;
	}

	while (1)
	{
		cout << "[view] set:1; lbi:2; director:3; referees:4; tokenkeys:5; token:6; user:7; block:8; exit:q; \n";
		cout << ">";
		cin >> s;
		n = cl::atoi(s, -1);
		if (s == "q")
			break;
		switch (n)
		{
		case 0:
			view_all(db);
			break;
		case 1:
			view_set(db);
			break;
		case 2:
			view_lbi(db);
			break;
		case 3:
			view_directors(db);
			break;
		case 4:
			view_referees(db);
			break;
		case 5:
			view_tokenkeys(db);
			break;
		case 6:
			view_token(db);
			break;
		case 7:
			view_user(db);
			break;
		case 8:
			view_block(db);
			break;
		default:
			break;
		}
	}

	delete db;
	cout << "view end!" << endl;
	return 0;
}

//*********************************************************
string& json_str(string& s, const chain_setting_t& i)
{
	char buf[1024];
	sprintf(buf, R"({
  "cid":%d,
  "setting":{
    "in_ex":%d,
    "tranfee":%llu,
    "consfee",%llu
  }
})", i.cid, (int)i.in_ex, (ull)i.min_tranfee, (ull)i.min_consfee);
	s = buf;
	return s;
}
string& json_str(string& s, const cpo_last_t& i)
{
	char buf[1024];
	sprintf(buf, R"({
	"height":%llu,
	"hash":"%s",
	"timestamp":"%s",
})", (ull)i.bt.height, i.bt.blockhash.to_hexString().c_str()
,cl::time_to_datetime_string(i.timestamp).c_str());
	s = buf;
	return s;
}
string& json_str(string& s, const tran_names_t& i)
{
	char buf[1024];
	sprintf(buf, R"({
  "height":%llu,
  "names":[)", (ull)i.height);
	s = buf;
	string str;
	if (!i.names.empty())
		s += "\n    \"" + cl::byte2hexs(i.names[0], str) + "\"";
	for(size_t j=1;j<i.names.size();++j)
		s += ",\n    \"" + cl::byte2hexs(i.names[j], str) + "\"";
	s += "\n  ]\n}";
	return s;
}
string& json_str(string& s, const list<string>& i)
{
	string str;
	s = R"({
  "list":[)";
	list<string>::const_iterator it = i.begin();
	if (it != i.end())
	{
		s += "\n    \"" + cl::byte2hexs(*it, str) + "\"";
		it++;
	}
	for (; it!=i.end(); ++it)
		s += ",\n    \"" + cl::byte2hexs(*it, str) + "\"";
	s += "\n  ]\n}";
	return s;
}
string& json_str(string& s, const tran_token_t& i)
{
	string s1, s2, s3, s4;
	char buf[10240];
	hash_trantoken((tran_token_t&)i);
	sprintf(buf, R"({
  "director_id":"%s",
  "hash":"%s",
  "id":"%s",
  "assets":%llu,
  "name":"%s",
  "description":"%s",
  "pic_size":%d
})",cl::byte2hexs(i.director_sign.id,s1).c_str()
,i.hash.to_hexString().c_str()
, cl::byte2hexs(i.id, s4).c_str(), (ull)i.assets,i.name.c_str(),i.description.c_str(),(int)i.pic.size());
	s = buf;
	return s;
}
string& json_str(string& s, const assets_t& i)
{
	char buf[1024];
	sprintf(buf, R"({
  "assets":%llu,
  "height":%llu
})", (ull)i.assets, (ull)i.height);
	s = buf;
	return s;
}
string& json_str(string& s, const block_t& i)
{
	hash_block_all((block_t&)i);
	char buf[10240];
	string s1, s2;
	size_t j;
	sprintf(buf, R"({
  "cid":%d,
  "height":%llu,
  "blockhash":"%s",
  "pre_hash":"%s",
  "timestamp":"%s",
  "pub_sign_size":%d,
  "vote_sign_num":%d,
  "pub_out":{"id":"%s","assets":%llu},
  "uas":[)", i.cid, (ull)i.height,i.blockhash.to_hexString().c_str()
,i.pre_hash.to_hexString().c_str(),cl::time_to_datetime_string(i.timestamp).c_str()
,(int)i.pub_sign.size(),(int)i.vote_signs.size()
,cl::byte2hexs(i.pub_out.id,s2).c_str(),(ull)i.pub_out.asset);
	s = buf;
	for (j = 0; j < i.uas.size(); ++j)
	{
		if (j == 0) s += "\n";
		else s += ",\n";
		sprintf(buf, R"(    {"id":"%s","assets":%llu,"height":%llu})",
			cl::byte2hexs(i.uas[j].id, s1).c_str(), (ull)i.uas[j].as.assets, (ull)i.uas[j].as.height);
		s += buf;
	}
	if (j) s += "\n";
	s += R"(  ],
  "tranhashs":[)";

	for (j = 0; j < i.tranhashs.size(); ++j)
	{
		if (j == 0) s += "\n";
		else s += ",\n";
		sprintf(buf, R"(    "%s")",
			i.tranhashs[j].to_hexString().c_str());
		s += buf;
	}
	if (j) s += "\n";
	s += R"(  ],
  "trans":[)";

	for (j = 0; j < i.trans.size(); ++j)
	{
		if (j == 0) s += "\n";
		else s += ",\n";
		trancert_to_str(s1, i.trans[j].data);
		s += s1;
	}
	if (j) s += "\n";
	s += "  ]\n}";
	return s;
}
string& trancert_to_str(string& s, const transaction_cert_t& t)
{
	char buf[10240];
	string s1;
	sprintf(buf, R"(    "%s|%llu|%llu|%s|%d|)", cl::b2h(t.to_asid()).c_str(), (ull)t.in_height, (ull)t.fee
		, cl::time_to_datetime_string(t.timestamp).c_str(), (int)t.type);
	
	if (TRT_TRANSFER == t.type)
	{
		for (size_t j = 0; j < t.outs.size(); j++)
		{
			sprintf(buf+strlen(buf),"(%s,%llu),"
				, cl::byte2hexs(t.outs[j].id, s1).c_str(), (ull)t.outs[j].asset);
		}
	}
	else if (TRT_DIRECTOR == t.type || TRT_REFEREE == t.type)
	{
		sprintf(buf + strlen(buf), "(%llu,", (ull)t.ns.height);
		for (size_t j = 0; j < t.ns.names.size(); j++)
		{
			sprintf(buf + strlen(buf), "%s,"
				, cl::byte2hexs(t.ns.names[j], s1).c_str());
		}
		sprintf(buf + strlen(buf), ")");
	}
	else if (TRT_TOKEN == t.type)
	{
		hash_trantoken((tran_token_t&)t.tok);
		sprintf(buf + strlen(buf), "%s,%llu,%s"
			, t.tok.hash.to_hexString().c_str(), (ull)t.tok.assets,t.tok.name.c_str());
	}
	else
	{
		sprintf(buf + strlen(buf), "***unkown");
	}
	sprintf(buf + strlen(buf), "\"");
	s = buf;
	return s;
}
//*********************************************************



void view_all(leveldb::DB *db)
{
	leveldb::ReadOptions rp;
	chain_setting_t set;
	cpo_last_t lbi;
	tran_names_t dirs, refs;
	list<string> tokenkeys;
	tran_token_t tok;
	assets_t ua;
	block_t b;
	uint64_t h;
	string k, s1;
	int num = 0;

#define EIF(k,v,name) else if (0 == memcmp(it->key().data(), k,min(it->key().size(),strlen(k)))) \
	{ \
		if (0 == string_to_struct(it->value().ToString(), v)) \
		{ \
			json_str(s1, v); \
			printf("[--%10d--%s--]\n", num,name); \
			printf("*[key]:%s\n", it->key().ToString().c_str()); \
			printf("*[val]:\n"); \
			cout << s1 << endl << endl; \
		} \
	}

	leveldb::Iterator *it = db->NewIterator(rp);
	for (it->SeekToFirst(); it->Valid(); it->Next()) 
	{
		num++;
		if (8 == it->key().size())
		{
			//block
			if (0 == string_to_struct(it->value().ToString(), b))
			{
				cl::string_to_struct(it->key().ToString(), h);
				json_str(s1, b);
				printf("[--%10d--block--]\n", num);
				printf("*[key]:%llu\n", (ull)h);
				printf("*[val]:\n");
				cout << s1 << endl << endl;
			}
		}
		else if (32 == it->key().size()||64== it->key().size())
		{
			//user
			if (0 == string_to_struct(it->value().ToString(), ua))
			{
				cl::byte2hexs(it->key().ToString(), k);
				json_str(s1, ua);
				printf("[--%10d--utxo--]\n",num);
				printf("*[key]:%s\n", k.c_str());
				printf("*[val]:\n");
				cout << s1 << endl << endl;
			}
		}
		else if (33 == it->key().size())
		{
			//token
			if (0 == string_to_struct(it->value().ToString(), tok))
			{
				cl::byte2hexs(it->key().ToString(), k);
				json_str(s1, tok);
				printf("[--%10d--token--]\n", num);
				printf("*[key]:%s\n", k.c_str());
				printf("*[val]:\n");
				cout << s1 << endl << endl;
			}
		}
		EIF(DBK_CHAINSETTING,set,"setting")
		EIF(DBK_LBI, lbi, "lbi")
		EIF(DBK_DIRECTORS, dirs, "directors")
		EIF(DBK_REFEREES, refs, "referees")
		EIF(DBK_TOKENKEYS, tokenkeys, "tokenkeys")
		else
		{
			printf("[--%10d--***** unkow k-v ******--]\n", num);
			printf("*[key]:%s\n\n", it->key().ToString().c_str());
		}
		getchar();
	}
	delete it;
}
#define PS(showkey) \
string s; \
if(showkey)printf("*[key]:%s\n", k.c_str()); \
printf("*[result]:\n"); \
if (db_readv(i, k, db)) cout << json_str(s, i) << endl << endl; \
else cout << "***no key\n";
void view_set(leveldb::DB *db)
{
	chain_setting_t i;
	string k = DBK_CHAINSETTING;
	PS(1);
}
void view_lbi(leveldb::DB *db)
{
	cpo_last_t i;
	string k = DBK_LBI;
	PS(1);
}
void view_directors(leveldb::DB *db)
{
	tran_names_t i;
	string k = DBK_DIRECTORS;
	PS(1);
}
void view_referees(leveldb::DB *db)
{
	tran_names_t i;
	string k = DBK_REFEREES;
	PS(1);
}
void view_tokenkeys(leveldb::DB *db)
{
	list<string> i;
	string k = DBK_TOKENKEYS;
	PS(1);
}
void view_token(leveldb::DB *db)
{
	tran_token_t i;
	string k,k1;
	while (1)
	{
		cout << "input tokenkey:\n>";
		cin >> k1;
		if (k1.empty())continue;
		if (k1 == "q")
			break;
		cl::hexs2byte(k1, k);
		PS(0);
	}
}
void view_user(leveldb::DB *db)
{
	assets_t i;
	string k,k1;
	while (1)
	{
		cout << "input userid:\n>";
		cin >> k1;
		if (k1.empty())continue;
		if (k1 == "q")
			break;
		cl::hexs2byte(k1, k);
		PS(0);
	}
}
void view_block(leveldb::DB *db)
{
	block_t i;
	string k;
	uint64_t h;
	while (1)
	{
		cout << "input height:\n>";
		cin >> k;
		if (k.empty())continue;
		if (k == "q")
			break;
		h = cl::atoll(k.c_str());
		struct_to_string(h, k, 8);
		PS(0);
	}
}
