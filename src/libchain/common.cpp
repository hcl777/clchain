#include "common.h"
#include "signature.h"
#include "aes128.h"
#include <iostream>

int operator << (bstream& s, const nodeaddr_t& i)
{
	s << i.id;
	s << i.pub;
	s << i.local;
	s << i.hport;
	s << i.nat_type;
	return s.ok();
}
int operator >> (bstream& s, nodeaddr_t& i)
{
	s >> i.id;
	s >> i.pub;
	s >> i.local;
	s >> i.hport;
	s >> i.nat_type;
	return s.ok();
}
int operator << (bstream& s, const ecuser_t& i)
{
	s << i.prik;
	s << i.pubk;
	return s.ok();
}
int operator >> (bstream& s, ecuser_t& i)
{
	s >> i.prik;
	s >> i.pubk;
	return s.ok();
}


comm::comm()
	: reactor(128)
	, m_is_referee(false)
{
}
comm::~comm()
{
}
int comm::init(const com_conf_t& c)
{
	int bsize[3];
	m_id = c.us.pubk;
	m_prikey = c.us.prik;
	wkdir = c.wkdir;
	bsize[0] = MB1K;
	bsize[1] = MB2K;
	bsize[2] = MB1M;
	mempool.init(bsize,3, &timer);

	na.id = m_id;
	na.nat_type = 0;
	na.local = c.local;
	na.pub = c.pub;
	na.hport = c.httpport;
	
	return 0;
}
void comm::fini()
{
	mempool.fini();
}


int comm::user_make(ecuser_t& u)
{
	string pri,pub;
	if (0 == cryp::sig::rand_key(pri, pub))
	{
		cl::byte2hexs(pri, u.prik);
		cl::byte2hexs(pub, u.pubk);
		return 0;
	}
	return -1;
}
int comm::user_save(const string& path, const string& ak, const ecuser_t& u)
{
	string s1, s2;
	cl::struct_to_string(u, s1, 1024);
	aes128::encrypt_s(ak, s1, s2);
	return cl::write_file_from_str(path, s2);
}
int comm::user_load(const string& path, const string& ak,  ecuser_t& u)
{
	string s1, s2;
	if (0 != cl::read_file_to_str(path, s1))
		return -1;
	if (0 == aes128::decrypt_s(ak, s1, s2))
		return cl::string_to_struct(s2,u);
	return -2;
}
