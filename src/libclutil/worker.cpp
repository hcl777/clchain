#include "worker.h"


namespace cl
{
	int worker::activate(int num/* = 1*/)
	{
		std::lock_guard<std::mutex> l(m_mt);
		if (!m_thrs.empty())
			return -1;
		m_brun = true;
		//以下lambda函数作为thread的参数传入，会在单独线程中执行。
		int n = 0;
		for (int i = 0; i < num; ++i)
		{
			n = 1;
			m_thrs.push_back(new std::thread([&]() {
				int e = i;
				n = 0;
				work(e); 
			}));
			while (0 != n); //等线程使用了i值后再执行下一个。
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
new std::thread(） 参数如果是对象非静态成员函数，可使用std::bind(func,obj) 绑定使用

*/

