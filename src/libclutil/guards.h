#pragma once

#include <mutex>

namespace cl
{

	using Mutex = std::mutex;
	using RecursiveMutex = std::recursive_mutex;
	using Guard = std::lock_guard<std::mutex>;
	using RGuard = std::lock_guard<std::recursive_mutex>;


}


/*
锁使用说明：
lock初始类型：
	默认：	阻塞获得
	adopt_lock : 假设已获得锁，不请求
	defer_lock : 不尝试请求，未获得
	try_to_lock: 尝试获得

std::condition_variable：
	--必须与unique_lock配合使用；
	--3种唤醒：获得通知，超时，虚假唤醒；
	--如果对wait的调用发生在notify之后是不会被唤醒的，
		所以接收者在使用wait等待之前也需要检查条件（标识）是否满足，另一个线程（通知者）在nofity前需要修改相应标识供接收者检查


*/






