#include "memblock.h"

#include "sorter.h"
namespace cl
{
	void memblock::release()
	{
		assert(ref>0);
		if (--ref == 0)
		{
			if (pool) pool->put_block(this);
			else delete this;
		}
	}

	//**************************** [memblockQueue] ****************************
	memblockQueue::memblockQueue(int i, int blocksize, int initnum, memblockPool* pool)
		:m_i(i)
		, m_blocksize(blocksize)
		, m_outs(0)
		, m_maxouts(0)
		, m_pool(pool)
	{
		for (int i = 0; i < initnum; ++i)
			m_ls.push_back(new memblock(i, blocksize,m_pool));
	}

	memblockQueue::~memblockQueue()
	{
		assert(0 == m_outs);
		for (BlockIter it = m_ls.begin(); it != m_ls.end(); ++it)
			delete *it;
		m_ls.clear();
	}

	void memblockQueue::put(memblock* b)
	{
		assert(b->i == m_i);
		b->rpos = b->wpos = 0;
		b->buflen = m_blocksize;
		m_ls.push_back(b);
		m_outs--;
	}
	memblock* memblockQueue::get()
	{
		memblock* b = NULL;
		if (!m_ls.empty())
		{
			b = m_ls.front();
			m_ls.pop_front();
		}
		else
		{
			b = new memblock(m_i, m_blocksize,m_pool);
		}
		m_outs++;
		if (m_outs>m_maxouts) m_maxouts = m_outs;
		return b;
	}

	void memblockQueue::reduce() //裁减,周期减
	{
		assert(m_maxouts >= m_outs);
		memblock *b = NULL;
		unsigned int n = m_maxouts - m_outs + 5; //总共块数最多保持最近最大需求量+5;
		int i = 0; //
		while (m_ls.size()>n)
		{
			b = m_ls.front();
			m_ls.pop_front();
			delete b;
			//一次释放10块
			if (++i >= 10)
				break;
		}
		m_maxouts = m_outs; //m_maxouts 最大需求量重置

	}


	//**************************** [memblockPool] ****************************

	memblockPool::memblockPool()
		:m_queue(NULL)
		, m_queue_num(0)
		, m_outs(0)
		, m_timer(NULL)
	{

	}
	memblockPool::~memblockPool()
	{
		assert(NULL == m_queue);
	}
	int memblockPool::init(int blocksize[], int n, timer* _timer)
	{
		m_queue_num = n;
		m_timer = _timer;
		//按升序排列.
		sort_bubble<int>(blocksize, n);
		if (m_queue_num)
		{
			m_queue = new memblockQueue*[m_queue_num];
			for (int i = 0; i<m_queue_num; ++i)
				m_queue[i] = new memblockQueue(i, blocksize[i],0,this);
			if(m_timer)
				m_timer->register_timer(static_cast<timerHandler*>(this), 1, 20000);
		}
		return 0;
	}
	void memblockPool::fini()
	{
		assert(m_outs == 0);
		if (m_queue)
		{
			if(m_timer)
				m_timer->unregister_all(static_cast<timerHandler*>(this));
			for (int i = 0; i<m_queue_num; ++i)
				delete m_queue[i];
			delete[] m_queue;
			m_queue = NULL;
			m_queue_num = 0;
		}
	}
	memblock* memblockPool::get_block(int size)
	{
		Lock l(m_mt);
		memblock *b = NULL;
		if (size <= 0)
			return NULL;
		for (int i = 0; i<m_queue_num; ++i)
		{
			if (size <= m_queue[i]->get_blocksize())
			{
				//在此仅提示用户预分配的定长内存块尺寸不适合
				assert(m_queue[i]->get_blocksize()<size + 2048);
				b = m_queue[i]->get();
				break;
			}
		}
		if (NULL == b)
		{
			//在此加assert()仅提示用户预计的定长内存块尺寸不适合
			assert(false);
			//-1表示游离的
			b = new memblock(-1, size,this);
		}
		m_outs++;
		b->refer();
		return b;
	}
	void memblockPool::put_block(memblock* b)
	{
		Lock l(m_mt);
		assert(b);
		if (!b)
			return;
		if (-1 == b->i)
			delete b;
		else
			m_queue[b->i]->put(b);
		m_outs--;
	}
	void memblockPool::on_timer(int e)
	{
		Lock l(m_mt);
		for (int i = 0; i<m_queue_num; ++i)
			m_queue[i]->reduce();
	}



}
