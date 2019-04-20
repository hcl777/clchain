#pragma once
#include <thread>
#include <vector>
#include <mutex>

namespace cl
{
	class worker
	{
	public:
		worker():m_brun(false){}
		virtual ~worker() {}

	protected:
		int activate(int num = 1);
		void wait();
		virtual void work(int i) = 0;

	protected:
		bool						m_brun;
	private:
		std::mutex					m_mt;
		std::vector<std::thread*>	m_thrs;
	};
}

