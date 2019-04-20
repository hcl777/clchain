#pragma once
#include <stdint.h>
#include <mutex>
#include "singleton.h"
namespace cl
{
	class timerHandler
	{
	public:
		timerHandler() {}
		virtual ~timerHandler() {}

		virtual void on_timer(int e) {}
	};



	class deleyQueue
	{
	public:
		deleyQueue();
		virtual ~deleyQueue();

		typedef struct tagTNode
		{
			timerHandler *h;
			int e; //eventid
			uint64_t us; //timeout
						  //long next_us;//下一次执行时间
			uint64_t remain_us; //剩余等待时间
			tagTNode *next;
			tagTNode *pre;
			tagTNode() { reset(); }
			void reset() { h = NULL; e = 0; us = 0;/*next_us=0;*/remain_us = 0; next = pre = this; }
		}TNode_t;

	public:
		uint64_t get_remain_us();
	protected:
		void add_node(TNode_t* node);
		void del_node(TNode_t* node);
		TNode_t* get_zero_delay();
		void synchronize();

	protected:
		TNode_t m_head;
		uint64_t m_last_tick;
		int m_size;
	};

	class timer : public deleyQueue
	{
	public:
		timer();
		virtual ~timer();
		typedef std::recursive_mutex Mutex; //可能在on_timer()中执行unregister_timer()重入的情况
		typedef std::lock_guard<Mutex> Lock;

	public:
		void handle_root();
		int register_timer(timerHandler *h, int e, long ms);
		int register_utimer(timerHandler *h, int e, uint64_t us);
		int unregister_timer(timerHandler *h, int e);
		int unregister_all(timerHandler *h);

	private:
		int find(timerHandler *h, int e);
		int allot();
		void free(int i);

	private:
		Mutex m_mt;
		TNode_t *m_nl;
		int m_nl_size;
		int m_cursor;
		bool m_is_free;
	};

	typedef singleton<timer> timerSngl;

}
