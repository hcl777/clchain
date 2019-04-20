#include "block.h"
#include "clsha3.h"

//可表示19位
#define MAX_TRAN_ASSETS (uint64_t)0x3fffffffffffffff


void hash_block_all(block_t& b)
{
	size_t i = 0;
	for (i = 0; i < b.trans.size(); ++i)
		hash_transaction(b.trans[i]);
	hash_block(b);
}
void hash_block(block_t& b)
{
	bstream s((int)(1024 + b.trans.size() * 35 + b.uas.size() * 50));
	block_hashdata_stream(s, b);
	sha3_256(b.blockhash.data(), 32, (uint8_t*)s.buffer(), s.length());
}


void assets_map_add(map<string, int64_t>& mp, const string& id, int64_t v)
{
	//checked
	map<string, int64_t>::iterator asit;
	asit = mp.find(id);
	if (asit == mp.end())
		mp[id] = v;
	else
		asit->second += v;
}
int make_block_assets(map<string, int64_t>& asmp, const tran_out_t& pub_out, const vector<transaction_t>& trans)
{
	assert(asmp.empty());
	size_t i,j;
	
	asmp[pub_out.id] = pub_out.asset;
	for (i = 0; i < trans.size(); ++i)
	{
		const transaction_cert_t& tc = trans[i].data;
		//if(tc.fee>0)
			assets_map_add(asmp, tc.id, 0 - (int64_t)tc.fee); 
		if (TRT_TRANSFER == tc.type)
		{
			for (j = 0; j < tc.outs.size(); ++j)
			{
				if (tc.outs[j].asset > MAX_TRAN_ASSETS)
					return BERR_ASSETS;
				//sub
				assets_map_add(asmp, tc.to_asid(), 0 - (int64_t)tc.outs[j].asset);
				//add,
				assets_map_add(asmp, tc.to_outid(tc.outs[j].id), (int64_t)tc.outs[j].asset);
			}
		}
		else if (TRT_TOKEN == tc.type)
		{
			//token assets
			assets_map_add(asmp, tc.id + tc.tok.hash.to_string(), tc.tok.assets);
		}
	}
	
	return 0;
}

int operator << (bstream& s, const chain_setting_t& i)
{
	s << i.cid;
	s << i.in_ex;
	s << i.min_tranfee;
	s << i.min_consfee;
	return s.ok();
}
int operator >> (bstream& s, chain_setting_t& i)
{
	s >> i.cid;
	s >> i.in_ex;
	s >> i.min_tranfee;
	s >> i.min_consfee;
	return s.ok();
}
int operator << (bstream& s, const assets_t& i)
{
	s << i.assets;
	s << i.height;
	return s.ok();
}
int operator >> (bstream& s, assets_t& i)
{
	s >> i.assets;
	s >> i.height;
	return s.ok();
}

int operator << (bstream& s, const user_assets_t& i)
{
	s << i.id;
	s << i.as;
	return s.ok();
}
int operator >> (bstream& s, user_assets_t& i)
{
	s >> i.id;
	s >> i.as;
	return s.ok();
}


int block_hashdata_stream(bstream& s, const block_t& i)
{
	s << i.cid;
	s << i.ver;
	s << i.height;
	s << i.pre_hash;
	s << i.timestamp;
	s << i.tranhashs;
	s << i.uas;
	s << i.pub_out;
	return s.ok();
}
int operator << (bstream& s, const block_t& i)
{
	block_hashdata_stream(s, i);
	s << i.vote_signs;
	s << i.pub_sign;
	s << i.trans;
	return s.ok();
}
int operator >> (bstream& s, block_t& i)
{
	s >> i.cid;
	s >> i.ver;
	s >> i.height;
	s >> i.pre_hash;
	s >> i.timestamp;
	s >> i.tranhashs;
	s >> i.uas;
	s >> i.pub_out;
	s >> i.vote_signs;
	s >> i.pub_sign;
	s >> i.trans;
	return s.ok();
}


