#include "worker.h"


namespace cl
{
	int worker::activate(int num/* = 1*/)
	{
		std::lock_guard<std::mutex> l(m_mt);
		if (!m_thrs.empty())
			return -1;
		m_brun = true;
		//����lambda������Ϊthread�Ĳ������룬���ڵ����߳���ִ�С�
		int n = 0;
		for (int i = 0; i < num; ++i)
		{
			n = 1;
			m_thrs.push_back(new std::thread([&]() {
				int e = i;
				n = 0;
				work(e); 
			}));
			while (0 != n); //���߳�ʹ����iֵ����ִ����һ����
		}
		return 0;
	}
	void worker::wait()
	{
		std::lock_guard<std::mutex> l(m_mt);
		for (std::vector<std::thread*>::iterator it = m_thrs.begin(); it != m_thrs.end(); ++it)
		{
			(*it)->join();
			delete (*it);
		}
		m_thrs.clear();
	}
}

/*
new std::thread(�� ��������Ƕ���Ǿ�̬��Ա��������ʹ��std::bind(func,obj) ��ʹ��

*/

