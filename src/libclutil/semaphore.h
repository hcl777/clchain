#pragma once

#include <mutex>
#include <condition_variable>

namespace cl
{
	class semaphore
	{
	public:
		semaphore(int sem=0)
			:_sem(0){}
		~semaphore(){}

		void signal() {
			std::lock_guard<std::mutex> l(_mt);
			_sem++;
			_cv.notify_one();
		}
		void wait() {
			std::unique_lock<std::mutex> l(_mt);
			//wait之后会解l的锁，并且参数2判断true即返回
			_cv.wait(l, [=] {return _sem > 0; });
			--_sem;
		}
	private:
		int _sem;
		std::mutex _mt;
		std::condition_variable _cv;
	};
}
