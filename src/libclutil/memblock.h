#pragma once
#include <list>
#include <mutex>
#include <assert.h>
#include "singleton.h"
#include "timer.h"
#include <string.h>

/*************************************
说明:
本内存池实现n组固定大小内存块的预分配管理.
每组的块大小相同.不同组块大小不相同.
本内存池操作不加锁.只适合单线程使用.
一个memblockQueue为一组相同大小的memblock..
一个memblockPool管理多个memblockQueue.
memblockQueue中的内存块会周期进行删减,每20秒执行一次.
**************************************/

namespace cl
{
#define MBLOCK_NEW_RETURN_INT(pool,name,size,ret) memblock *name = pool->get_block(size); if(NULL==name) {return ret;} 
	class memblockPool;
	class memblock
	{
		friend class memblockQueue;
		friend class memblockPool;
	public:
		char *buf;
		int buflen, rpos, wpos;
	private:
		int i;
		int ref;
		memblockPool* pool;
	private:
		memblock(int i, int size, memblockPool* _pool)
			:buf(new char[size])
			, buflen(size)
			, rpos(0)
			, wpos(0)
			, i(i)
			, ref(0)//初始0
			, pool(_pool)
		{
		}
		~memblock()
		{
			delete[] buf;
		}

	public:
		void refer() { ref++; }
		void release();
		int length() const { return wpos - rpos; }
		char* read_ptr() const { return buf + rpos; }
		int write(const char* b, int len, int offset)
		{
			if (offset>wpos)
				return 0;
			int tmp = wpos - offset; //去掉重叠部分
			if (len <= tmp)
				return 0;
			len -= tmp;
			b += tmp;
			assert(wpos + len <= buflen);
			if (wpos + len>buflen) len = buflen - wpos;
			memcpy(buf + wpos, b, len);
			wpos += len;
			return len;
		}
	};
	class memblockQueue
	{
	public:
		memblockQueue(int i, int blocksize,int initnum, memblockPool* pool);
		~memblockQueue();

		typedef std::list<memblock*> BlockList;
		typedef BlockList::iterator BlockIter;
	public:
		void put(memblock* b);
		memblock* get();

		void reduce(); //裁减,周期减
		int get_blocksize() const { return m_blocksize; }
	private:
		BlockList m_ls;
		int m_i;
		int m_blocksize;	//block 大小
		int m_outs;			//已经调出去的.
		int m_maxouts;		//最近周期内最大需求量
		memblockPool* m_pool;
	};

	class memblockPool : public timerHandler
	{
		typedef std::mutex Mutex;
		typedef std::unique_lock<Mutex> Lock;
	public:
		memblockPool();
		~memblockPool();
	public:
		int init(int blocksize[], int n,timer* _timer);
		void fini();

		memblock* get_block(int size);
		void put_block(memblock* b);

		virtual void on_timer(int e);
	private:
		Mutex	m_mt;
		memblockQueue **m_queue;
		int m_queue_num;
		int m_outs;
		timer* m_timer;
	};

	//typedef singleton<memblockPool> memblockPoolSngl;
}
