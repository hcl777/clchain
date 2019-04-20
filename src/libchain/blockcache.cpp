#include "blockcache.h"



blockcache::blockcache()
{
}


blockcache::~blockcache()
{
}
int blockcache::init()
{
	return 0;
}
void blockcache::fini()
{
	for (map<uint64_t, string*>::iterator it = m_mp.begin(); it != m_mp.end(); ++it)
		delete it->second;
	m_mp.clear();
}
int blockcache::put(uint64_t height, char* buf, int size)
{
	Guard g(m_mt);
	if (m_mp.find(height) == m_mp.end())
		m_mp[height] = new string(buf, size);
	return 0;
}
string* blockcache::get(uint64_t height)
{
	Guard g(m_mt);
	string* s = NULL;
	map<uint64_t, string*>::iterator it = m_mp.find(height);
	if (it != m_mp.end())
	{
		s = it->second;
		m_mp.erase(it);
	}
	return s;
}
void blockcache::del_old(uint64_t height)
{
	Guard g(m_mt);
	for (map<uint64_t, string*>::iterator it = m_mp.begin(); it != m_mp.end(); )
	{
		if (it->first > height)
			break;
		delete it->second;
		m_mp.erase(it++);
	}
}
int blockcache::get_sync_task(uint64_t minh, uint64_t maxh, list<uint64_t>& ls)
{
	uint64_t i = minh;
	for (map<uint64_t, string*>::iterator it = m_mp.begin(); it != m_mp.end(); )
	{
		while (i < it->first)
		{
			ls.push_back(i++);
			if (i > maxh)
				break;
		}
		if (i > maxh)
			break;
		i++;
	}
	while (i <= maxh)
		ls.push_back(i++);
	return 0;
}
