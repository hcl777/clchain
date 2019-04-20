#include "cltype.h"
#include <time.h>
#include <chrono>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN		// �� Windows ͷ���ų�����ʹ�õ�����
#define NOMINMAX		//Vc��max() min()����stl��ĳ�ͻ,����VC��
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
//			QueryPerformanceFrequency(&litmp);//���ʱ��Ƶ��
//			rate = litmp.QuadPart;
//			QueryPerformanceCounter(&litmp);//��ó�ʼֵ
//			begin_count = litmp.QuadPart;
//		}
//		QueryPerformanceCounter(&litmp);//
//		curr_count = litmp.QuadPart - begin_count;
//		tick = (curr_count / rate) * 1000000 + (uint64_t)((curr_count%rate) / (double)rate * 1000000);
//		//tick = (long long)(curr_count/(double)rate * 1000000);
//		return tick;
//#else
//		//ʹ��clock_gettime ����ʹ�� -lrt. ʹ��rt��
//		static struct timespec ts;
//		clock_gettime(CLOCK_MONOTONIC, &ts);
//		return (uint64_t)(ts.tv_sec * (uint64_t)1000000 + ts.tv_nsec / 1000);
//#endif
//	}

	//linux ��˯��0����ʵ������Ϊ1���룬˯��1����ʱ����Ҫ2���������
	//windows ��˯��0����ʵ��Ϊæ�ȴ���˯��1����ʱ����Ҫ2���������
	void msleep(uint32_t millisec)
	{
#ifdef _MSC_VER
		Sleep(millisec);
#elif defined(__GNUC__)
		usleep(millisec * 1000);
#endif
		//std::this_thread::sleep_for(std::chrono::milliseconds(millisec)); //WIN��ϵͳʱ�������Сʱʱ��һֱ������׼ȷ
	}

	const char* stristr(const char * inBuffer, const  char * inSearchStr)
	{
		const char*  currBuffPointer = inBuffer;

		while (*currBuffPointer != 0x00)
		{
			const char* compareOne = currBuffPointer;
			const char* compareTwo = inSearchStr;
			//ͳһת��ΪСд�ַ�
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
//		int c = getchar(); //��Ҫ�Ȼس������Բ�����
//		system("stty echo");
//		return c;
//#endif // 
//
//	}
}




