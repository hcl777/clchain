#include "cltype.h"
#include <time.h>
#include <chrono>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
#define NOMINMAX		//Vc的max() min()宏与stl库的冲突,禁用VC的
#include <windows.h>
//#include <conio.h>
#else
#include <unistd.h>
#endif

namespace cl
{
	uint64_t utick() { return std::chrono::steady_clock::now().time_since_epoch().count() / 1000; }
//	uint64_t utick()
//	{
//#ifdef _MSC_VER
//		static bool b = false;
//		static uint64_t begin_count = 0;
//		static uint64_t rate = 0;
//		static uint64_t curr_count = 0;
//		static uint64_t tick = 0;
//		static uint64_t tmp = 0;
//		static LARGE_INTEGER litmp;
//		if (!b)
//		{
//			b = true;
//			QueryPerformanceFrequency(&litmp);//获得时钟频率
//			rate = litmp.QuadPart;
//			QueryPerformanceCounter(&litmp);//获得初始值
//			begin_count = litmp.QuadPart;
//		}
//		QueryPerformanceCounter(&litmp);//
//		curr_count = litmp.QuadPart - begin_count;
//		tick = (curr_count / rate) * 1000000 + (uint64_t)((curr_count%rate) / (double)rate * 1000000);
//		//tick = (long long)(curr_count/(double)rate * 1000000);
//		return tick;
//#else
//		//使用clock_gettime 必须使用 -lrt. 使用rt库
//		static struct timespec ts;
//		clock_gettime(CLOCK_MONOTONIC, &ts);
//		return (uint64_t)(ts.tv_sec * (uint64_t)1000000 + ts.tv_nsec / 1000);
//#endif
//	}

	//linux 下睡眠0毫秒实际周期为1毫秒，睡眠1毫秒时基本要2毫秒的周期
	//windows 下睡眠0毫秒实际为忙等待，睡眠1毫秒时基本要2毫秒的周期
	void msleep(uint32_t millisec)
	{
#ifdef _MSC_VER
		Sleep(millisec);
#elif defined(__GNUC__)
		usleep(millisec * 1000);
#endif
		//std::this_thread::sleep_for(std::chrono::milliseconds(millisec)); //WIN当系统时间调整变小时时会一直阻塞不准确
	}

	const char* stristr(const char * inBuffer, const  char * inSearchStr)
	{
		const char*  currBuffPointer = inBuffer;

		while (*currBuffPointer != 0x00)
		{
			const char* compareOne = currBuffPointer;
			const char* compareTwo = inSearchStr;
			//统一转换为小写字符
			while (tolower(*compareOne) == tolower(*compareTwo))
			{
				compareOne++;
				compareTwo++;
				if (*compareTwo == 0x00)
				{
					return currBuffPointer;
				}

			}
			currBuffPointer++;
		}
		return NULL;
	}
//	int cl_getch()
//	{
//#ifdef _MSC_VER
//		return _getch();
//#else
//		system("stty -echo");
//		int c = getchar(); //需要等回车，所以不适用
//		system("stty echo");
//		return c;
//#endif // 
//
//	}
}




