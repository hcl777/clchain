#include "genesis.h"
#include "util.h"
#include "json/json.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"



using namespace std;

int genesis_load_json(const std::string& path, block_t& b)
{
	Json::Reader rd;
	Json::Value root,v;
	Json::ValueIterator it;
	transaction_t tran;
	string s, s1, s2;
	signature_t sig;
	
	if (0 != cl::read_file_to_str(path, s))
		return -1;
	if (!rd.parse(s.c_str(), root))
		return -1;

	b.height = 1;
	b.timestamp = time(NULL);
	//b.cid = root["cid"].asUInt();
	sig.sign = root["setting"].toStyledString();
	b.pub_out.asset = root["assets"].asInt64();
	v = root["directors"];
	for (it = v.begin(); it != v.end(); ++it)
	{
		s1 = (*it).asCString();
		cl::hexs2byte(s1, s2);
		if(s2.size()== IDSIZE)
			tran.data.ns.names.push_back(s2);
	}
	if (tran.data.ns.names.empty())
		return -2;
	tran.data.ns.height = 0;
	b.pub_out.id = tran.data.ns.names[0];//默认第1
	b.vote_signs.push_back(sig);
	tran.data.type = TRT_DIRECTOR;
	tran.data.timestamp = b.timestamp;
	b.trans.push_back(tran);

	//签名：
	//第一块的CID为0
	b.cid = 0;
	hash_block(b);
	char pass[1024];
	string prikey;
	for (int i = 0; i < 3; ++i)
	{
		printf("enter your prikey:");
		get_scanf_pass(pass,1024);
		prikey = h2b(pass);
		if (cryp::sig::check_key(prikey, b.pub_out.id))
		{
			cryp::sig::sign(prikey, b.blockhash, b.pub_sign);
			break;
		}
	}
	return cryp::sig::verify(b.pub_out.id, b.pub_sign, b.blockhash) ? 0 : -1;
}
int genesis_chain_setting(const std::string& jsonstr, chain_setting_t& set)
{
	Json::Reader rd;
	Json::Value root;
	if (!rd.parse(jsonstr.c_str(), root))
		return -1;
	set.in_ex = root["in_ex"].asUInt();
	set.min_consfee = root["consfee"].asUInt();
	set.min_tranfee = root["tranfee"].asUInt();
	return 0;
}
