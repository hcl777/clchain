#include "cproto.h"
#include "clsha3.h"



int operator << (bstream& s, const pc_rsp_block_t& i)
{
	s << i.height;
	s << i.result;
	s << i.s;
	return s.ok();
}
int operator >> (bstream& s, pc_rsp_block_t& i)
{
	s >> i.height;
	s >> i.result;
	s >> i.s;
	return s.ok();
}

void hash_handshake(pc_handshake_t& i)
{
	bstream s(1024); //
	s << i.my;
	s << i.des;
	s << i.t;
	sha3_256(i.hash.data(), 32, (uint8_t*)s.buffer(), s.length());
}
int operator << (bstream& s, const pc_handshake_t& i)
{
	s << i.my;
	s << i.des;
	s << i.t;
	s << i.sign;
	return s.ok();
}
int operator >> (bstream& s, pc_handshake_t& i)
{
	s >> i.my;
	s >> i.des;
	s >> i.t;
	s >> i.sign;
	return s.ok();
}


int operator << (bstream& s, const pc_rsp_propose_t& i)
{
	s << i.result;
	s << i.timestamp;
	s << i.sign;
	return s.ok();
}
int operator >> (bstream& s, pc_rsp_propose_t& i)
{
	s >> i.result;
	s >> i.timestamp;
	s >> i.sign;
	return s.ok();
}

int operator << (bstream& s, const pc_proposets_t& i)
{
	s << i.id;
	s << i.timestamp;
	s << i.sign;
	return s.ok();
}
int operator >> (bstream& s, pc_proposets_t& i)
{
	s >> i.id;
	s >> i.timestamp;
	s >> i.sign;
	return s.ok();
}
int operator << (bstream& s, const pc_verify_propose_t& i)
{
	s << i.height;
	s << i.signs;
	return s.ok();
}
int operator >> (bstream& s, pc_verify_propose_t& i)
{
	s >> i.height;
	s >> i.signs;
	return s.ok();
}

int operator << (bstream& s, const pc_rsp_blocksign_t& i)
{
	s << i.result;
	s << i.height;
	s << i.sign;
	return s.ok();
}
int operator >> (bstream& s, pc_rsp_blocksign_t& i)
{
	s >> i.result;
	s >> i.height;
	s >> i.sign;
	return s.ok();
}

int operator << (bstream& s, const pc_pub_blocksign_t& i)
{
	s << i.bt;
	s << i.vote_signs;
	return s.ok();
}
int operator >> (bstream& s, pc_pub_blocksign_t& i)
{
	s >> i.bt;
	s >> i.vote_signs;
	return s.ok();
}

