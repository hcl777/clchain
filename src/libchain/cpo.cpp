#include "cpo.h"


int operator << (bstream& s, const blocktag_t& i)
{
	s << i.height;
	s << i.blockhash;
	return s.ok();
}
int operator >> (bstream& s, blocktag_t& i)
{
	s >> i.height;
	s >> i.blockhash;
	return s.ok();
}

int operator << (bstream& s, const cpo_last_t& i)
{
	s << i.bt;
	s << i.timestamp;
	return s.ok();
}
int operator >> (bstream& s, cpo_last_t& i)
{
	s >> i.bt;
	s >> i.timestamp;
	return s.ok();
}


