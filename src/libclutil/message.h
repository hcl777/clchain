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
		~message() { if (psem) psem->signal(); } //��Ӧ��Ϣ��ִ����Ϣɾ��������sendʱ��ȴ�
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

		message* get();//����
		message* tryget(); //
		void send(message* msg);//����ֱ��msg��ɾ��
		void post(message* msg); //
		void clear(void(*f)(message*) = 0);
	private:
		Mutex					m_mt;
		semaphore				m_sem;
		std::list<message*>		m_ls;
	};

}

