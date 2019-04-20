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
��ʹ��˵����
lock��ʼ���ͣ�
	Ĭ�ϣ�	�������
	adopt_lock : �����ѻ������������
	defer_lock : ����������δ���
	try_to_lock: ���Ի��

std::condition_variable��
	--������unique_lock���ʹ�ã�
	--3�ֻ��ѣ����֪ͨ����ʱ����ٻ��ѣ�
	--�����wait�ĵ��÷�����notify֮���ǲ��ᱻ���ѵģ�
		���Խ�������ʹ��wait�ȴ�֮ǰҲ��Ҫ�����������ʶ���Ƿ����㣬��һ���̣߳�֪ͨ�ߣ���nofityǰ��Ҫ�޸���Ӧ��ʶ�������߼��


*/






