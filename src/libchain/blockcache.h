#pragma once
#include "block.h"
#include "guards.h"
#include "leveldb/db.h"

class blockcache
{
public:
	blockcache();
	~blockcache();

public:
	int init();
	void fini();
	int put(uint64_t height,char* buf, int size);
	string* get(uint64_t height);
	void del_old(uint64_t height);
	int get_sync_task(uint64_t minh, uint64_t maxh, list<uint64_t>& ls);
	bool exist(uint64_t height)
	{
		Guard g(m_mt);
		return m_mp.find(height) != m_mp.end();
	}
	
private:
	//暂用map记录已cache的块，以后改用滑动bittable方式优化查询速度。
	Mutex						m_mt;
	map<uint64_t, string*>		m_mp;
	
};

