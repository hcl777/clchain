#include "btrans.h"

#include "clsha3.h"

void hash_transaction(transaction_t& i)
{
	bstream s(102400); //最大100K
	s << i.data;
	sha3_256(i.tranhash.data(), 32, (uint8_t*)s.buffer(), s.length());
	if (TRT_TOKEN == i.data.type)
		hash_trantoken(i.data.tok);
}
void hash_trantoken(tran_token_t& i)
{
	bstream s(102400); //最大100K
	tran_token_hashdata_stream(s, i);
	sha3_256(i.hash.data(), 32, (uint8_t*)s.buffer(), s.length());
}

int operator << (bstream& s, const signature_t& i)
{
	s << i.id;
	s << i.sign;
	return s.ok();
}
int operator >> (bstream& s, signature_t& i)
{
	s >> i.id;
	s >> i.sign;
	return s.ok();
}
//int operator << (bstream& s, const tran_key_t& i)
//{
//	s << i.id;
//	s << i.pubkey;
//	return s.ok();
//}
//int operator >> (bstream& s, tran_key_t& i)
//{
//	s >> i.id;
//	s >> i.pubkey;
//	return s.ok();
//}
int operator << (bstream& s, const tran_names_t& i)
{
	s << i.height;
	s << i.names;
	//s << i.timestamp;
	return s.ok();
}
int operator >> (bstream& s, tran_names_t& i)
{
	s >> i.height;
	s >> i.names;
	//s >> i.timestamp;
	return s.ok();
}


int operator << (bstream& s, const tran_out_t& i)
{
	s << i.id;
	s << i.asset;
	return s.ok();
}
int operator >> (bstream& s, tran_out_t& i)
{
	s >> i.id;
	s >> i.asset;
	return s.ok();
}

int tran_token_hashdata_stream(bstream& s, const tran_token_t& i)
{
	s << i.cid;
	s << i.id;
	s << i.assets;
	s << i.name;
	s << i.description;
	s << i.pic;
	return s.ok();
}
int operator << (bstream& s, const tran_token_t& i)
{
	tran_token_hashdata_stream(s, i);
	s << i.pub_sign;
	s << i.director_sign;
	return s.ok();
}
int operator >> (bstream& s, tran_token_t& i)
{
	s >> i.cid;
	s >> i.id;
	s >> i.assets;
	s >> i.name;
	s >> i.description;
	s >> i.pic;
	s >> i.pub_sign;
	s >> i.director_sign;
	return s.ok();
}

int operator << (bstream& s, const transaction_cert_t& i)
{
	s << i.cid;
	s << i.id;
	s << i.tid;
	s << i.in_height;
	//s << i.as_height;
	s << i.fee;
	s << i.timestamp;
	s << i.ext;
	s << i.type;
	if (TRT_TRANSFER == i.type) s << i.outs;
	else if (TRT_DIRECTOR == i.type || TRT_REFEREE == i.type) s << i.ns;
	else if (TRT_TOKEN == i.type) s << i.tok;
	else
	{
		assert(0);
	}
	return s.ok();
}
int operator >> (bstream& s, transaction_cert_t& i)
{
	s >> i.cid;
	s >> i.id;
	s >> i.tid;
	s >> i.in_height;
	//s >> i.as_height;
	s >> i.fee;
	s >> i.timestamp;
	s >> i.ext;
	s >> i.type;
	if (TRT_TRANSFER == i.type) s >> i.outs;
	else if (TRT_DIRECTOR == i.type || TRT_REFEREE == i.type) s >> i.ns;
	else if (TRT_TOKEN == i.type) s >> i.tok;
	else
	{
		assert(0);
	}
	return s.ok();
}
int operator << (bstream& s, const transaction_t& i)
{
	s << i.data;
	s << i.sign;
	return s.ok();
}

int operator >> (bstream& s, transaction_t& i)
{
	s >> i.data;
	s >> i.sign;
	return s.ok();
}

namespace trans
{

	int new_tran(transaction_t& t, uint32_t cid, ecuser_t& u, const string& tokid, uint64_t fee, uint64_t in_height, vector<tran_out_t>& outs, const string& ext)
	{
		transaction_cert_t& c = t.data;
		if (!cryp::sig::check_key(u.prik, u.pubk))return -11;
		if (outs.empty()) return -12;
		c.cid = cid;
		c.id = u.pubk;
		c.tid =tokid;
		c.outs = outs;
		c.fee = fee;
		c.in_height = in_height;
		c.timestamp = time(NULL);
		c.ext = ext;
		c.type = TRT_TRANSFER;

		hash_transaction(t);
		cryp::sig::sign(u.prik, t.tranhash, t.sign);
		return 0;
	}
	int new_names(transaction_t& t, uint32_t cid, ecuser_t& u, uint64_t in_height, tran_names_t& ns, uint8_t type)
	{
		transaction_cert_t& c = t.data;
		if (!cryp::sig::check_key(u.prik, u.pubk))return -11;
		if (ns.names.empty()) return -12;
		if (type != TRT_DIRECTOR && type != TRT_REFEREE)
			return -13;
		c.cid = cid;
		c.id = u.pubk;
		c.tid = "";
		c.fee = 0;
		c.in_height = in_height;
		c.timestamp = time(NULL);
		c.type = type;
		c.ns = ns;

		hash_transaction(t);
		cryp::sig::sign(u.prik, t.tranhash, t.sign);
		return 0;

	}
	int new_token(transaction_t& t, uint32_t cid, ecuser_t& u, ecuser_t& dire, uint64_t in_height,
		uint64_t assets, const string& name, const string& description, uint64_t fee)
	{
		transaction_cert_t& c = t.data;
		tran_token_t& tok = c.tok;
		if (!cryp::sig::check_key(u.prik, u.pubk))return -11;
		if (!cryp::sig::check_key(dire.prik, dire.pubk))return -11;

		c.cid = cid;
		c.id = u.pubk;
		c.fee = fee;
		c.in_height = in_height;
		c.timestamp = time(NULL);
		c.type = TRT_TOKEN;

		tok.cid = cid;
		tok.id = u.pubk;
		tok.assets = assets;
		tok.name = name;
		tok.description = description;
		hash_trantoken(tok);
		cryp::sig::sign(u.prik, tok.hash, tok.pub_sign);
		tok.director_sign.id = dire.pubk;
		cryp::sig::sign(dire.prik, tok.hash, tok.director_sign.sign);

		hash_transaction(t);
		cryp::sig::sign(u.prik, t.tranhash, t.sign);
		return 0;
	}
}

