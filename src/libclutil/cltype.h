#pragma once
#include <stdint.h>
#include "slice.h"
#include <algorithm>

/*
操作系统平台区别 defined()
VC:			_MSC_VER
GCC:		__GNUC__
linux:		__linux__
uinx:		__unix__
android:	ANDROID
*/

//注意：long类型x64: window为4B（兼容性更好LLP64），linux为8B (LP64)。


/*
4.8版本gcc有bug
GCC 4.8 does not work because of two bugs (55817 and 57824) in the C++11 support. Note there is a pull request to fix some of the issues.
GCC 4.8 has a bug 57824): multiline raw strings cannot be the arguments to macros.
	Don't use multiline raw strings directly in macros with this compiler.
*/
#if defined(__GNUC__) && !(defined(__ICC) || defined(__INTEL_COMPILER))
#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) < 40900
#error "unsupported GCC version - see https://github.com/nlohmann/json#supported-compilers"
#endif
#endif

#if defined(__cplusplus) && !defined(CLBEGIN)
#define CLBEGIN namespace cl{
#define CLEND	}
#define CL	::cl::
#endif

#if defined(__GNUC__)
#define sscanf_s sscanf
#define stricmp strcasecmp
#endif

namespace cl
{
#ifndef STATIC_ASSERT
#define STATIC_ASSERT(expr) typedef char __static_assert_t[ (expr) ]
#endif

#define GETSET(type, name, name2) \
protected: type name; \
public: type& get##name2() { return name; } \
	void set##name2(type const & a##name2) { name = a##name2; }

#define GET(type, name, name2) \
protected: type name; \
public: type& get##name2() { return name; }


	inline void void_printf(const char* _Format, ...) {}
#ifdef ANDROID
#include <jni.h>
#include <android/log.h>

#define DEBUGMSG void_printf
#define CLLOG1 void_printf
	//#define DEBUGMSG(...)  __android_log_print(ANDROID_LOG_DEBUG,"#JNI:",__VA_ARGS__) // 定义LOGD类型
	//#define CLLOG1(...)  __android_log_print(ANDROID_LOG_DEBUG,"#JNI:",__VA_ARGS__) // 定义LOGD类型
#else
	//#define DEBUGMSG void_printf
#define DEBUGMSG printf
#define D printf("*--FILE[%s] FUNC[%s] LINE[%d]--*\n",__FILE__,__FUNCTION__,__LINE__);
#define CLLOG1 printf
#endif

	using ll = long long;
	using ull = unsigned long long;

// //std::min max 宏会与windows的冲突
//#ifdef  _MSC_VER
//#ifndef max
//#define max(a,b)            (((a) > (b)) ? (a) : (b))
//#endif
//
//#ifndef min
//#define min(a,b)            (((a) < (b)) ? (a) : (b))
//#endif
//#endif

	//假定时间无限增加大，不会重置.
	inline uint32_t time_distance(uint32_t after, uint32_t begin) { if (after > begin) return after - begin; else return after + ((uint32_t)-1 - begin) + 1; }
	inline uint64_t time_distance(uint64_t after, uint64_t begin) { if (after > begin) return after - begin; else return after + ((uint64_t)-1 - begin) + 1; }
	
	//假设两个时间tick相差不大，会重置
	inline bool time_after(uint32_t t, uint32_t base) { return (((int32_t)t - (int32_t)base) >= 0); }
	inline bool time_after(uint64_t t, uint64_t base) { return (((int64_t)t - (int64_t)base) >= 0); }

	//clock 在虚拟机下使用不正确(CLOCKS_PER_SEC = 1000)
	uint64_t utick();
	inline uint64_t mtick() { return utick() / 1000; }
	
	void msleep(uint32_t millisec);


	const char* stristr(const char * inBuffer, const  char * inSearchStr);
	//int cl_getch();
}

