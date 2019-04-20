#include "message.h"

namespace cl
{

	msgQueue::msgQueue()
	{
	}


	msgQueue::~msgQueue()
	{
	}
	message* msgQueue::get()
	{
		m_sem.wait();
		return tryget();
	}
	message* msgQueue::tryget()
	{
		message* msg = NULL;
		Guard g(m_mt);
		if (!m_ls.empty())
		{
			msg = m_ls.front();
			m_ls.pop_back();
		}
		return msg;
	}
	void msgQueue::send(message* msg)
	{
		semaphore sem;
		msg->psem = &sem;
		post(msg);
		sem.wait();
	}
	void msgQueue::post(message* msg)
	{
		{
			Guard g(m_mt);
			m_ls.push_back(msg);
		}
		m_sem.signal();
	}
	void msgQueue::clear(void(*f)(message*)/* = 0*/)
	{
		Guard	g(m_mt);
		if (f)
			for (std::list<message*>::iterator it = m_ls.begin(); it != m_ls.end(); ++it)
				f(*it);
		m_ls.clear();
	}
}
