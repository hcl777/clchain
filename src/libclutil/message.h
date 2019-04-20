#pragma once
#include "guards.h"
#include "semaphore.h"
#include <list>

namespace cl
{
	class message
	{
		friend class msgQueue;
	public:
		message(int _cmd, void* _data,void* _res=NULL) :cmd(_cmd), data(_data),res(_res), psem(NULL) {}
		~message() { if (psem) psem->signal(); } //响应消息的执行消息删除操作，send时会等待
	public:
		int			cmd;
		void*		data;
		void*		res;
	private:
		semaphore * psem;
	};


	class msgQueue
	{
	public:
		msgQueue();
		~msgQueue();

		message* get();//阻塞
		message* tryget(); //
		void send(message* msg);//阻塞直到msg被删除
		void post(message* msg); //
		void clear(void(*f)(message*) = 0);
	private:
		Mutex					m_mt;
		semaphore				m_sem;
		std::list<message*>		m_ls;
	};

}

