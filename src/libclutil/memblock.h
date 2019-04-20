#pragma once
#include <list>
#include <mutex>
#include <assert.h>
#include "singleton.h"
#include "timer.h"
#include <string.h>

/*************************************
˵��:
���ڴ��ʵ��n��̶���С�ڴ���Ԥ�������.
ÿ��Ŀ��С��ͬ.��ͬ����С����ͬ.
���ڴ�ز���������.ֻ�ʺϵ��߳�ʹ��.
һ��memblockQueueΪһ����ͬ��С��memblock..
һ��memblockPool������memblockQueue.
memblockQueue�е��ڴ������ڽ���ɾ��,ÿ20��ִ��һ��.
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
			, ref(0)//��ʼ0
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
			int tmp = wpos - offset; //ȥ���ص�����
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

		void reduce(); //�ü�,���ڼ�
		int get_blocksize() const { return m_blocksize; }
	private:
		BlockList m_ls;
		int m_i;
		int m_blocksize;	//block ��С
		int m_outs;			//�Ѿ�����ȥ��.
		int m_maxouts;		//������������������
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
