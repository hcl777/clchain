#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <stdarg.h>
#include "crc32.h"
#include <stdio.h>


#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
#include <windows.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <conio.h>
#elif defined(__GNUC__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

using namespace std;

CLBEGIN

	void debug_memleak()
	{
#ifdef _MSC_VER
		//指定输出到执行程序的控制台
		//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE); 
		//_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

		//1。开启报告内存泄漏开关，退出前会自动调用_CrtDumpMemoryLeaks()
		//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |_CRTDBG_LEAK_CHECK_DF)
		//_CRTDBG_LEAK_CHECK_DF:指在程序退出时调用函数_CrtDumpMemoryLeaks
		_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
		
		//2。打印调用这函数之前的所有未释放泄漏。不应在此调用。默认程序结束时会调用。
		//_CrtDumpMemoryLeaks(); //执行这个会有164B内存泄漏

		//3。设置泄漏处中断，根据泄漏的序号中断（第几次new的内存，在dump中如{162} noral...）
		//_CrtSetBreakAlloc(162);
#endif
	}
	int chang_dir(const char* path)
	{
#ifdef _MSC_VER
		return TRUE == ::SetCurrentDirectoryA(path);
#else
		return chdir(path);
#endif
	}
	int _GetModuleFileName(const char* lpszModuleName, char* lpszModulePath, int cbModule)
	{
#ifdef _MSC_VER
		return ::GetModuleFileNameA(::GetModuleHandleA(lpszModuleName), lpszModulePath, cbModule);
#else
		char exelink[128];
		sprintf(exelink, "/proc/%d/exe", getpid());
		int count = readlink(exelink, lpszModulePath, cbModule);
		if (count>0)
			lpszModulePath[count] = '\0';
		else
			lpszModulePath[0] = '\0';
		return count;
#endif
	}
	std::string get_module_path()
	{
		char buf[256] = { 0, };
		_GetModuleFileName(NULL, buf, 256);
		return buf;
	}
	std::string get_module_dir()
	{
		char buf[256] = { 0, };
		if (_GetModuleFileName(NULL, buf, 256))
		{
			char *p = ::strrchr(buf, '/');
			if (!p)
				p = ::strrchr(buf, '\\');
			if (p)
				*(p + 1) = '\0';
		}
		return buf;
	}
	std::string get_module_name()
	{
		char buf[256] = { 0, };
		if (_GetModuleFileName(NULL, buf, 256))
		{
			char *p = ::strrchr(buf, '/');
			if (!p)
				p = ::strrchr(buf, '\\');
			if (p)
				return (p + 1);
		}
		return buf;
	}
	std::string& dir_format(std::string& dir)
	{
		if (dir.empty())return dir;
		char& c = dir.at(dir.size() - 1);
		if (c != '/'&&c != '\\')
			dir += '/';
		return dir;
	}
	int create_dir(const std::string& dir)
	{
		if (dir.empty())
			return -1;
		string str = dir;
		size_t pos = str.find("\\");
		while (pos != string::npos)
		{
			str.replace(pos, 1, "/", 1);
			pos = str.find("\\");
		}
		if (str.substr(str.length() - 1) == "/")
			str = str.substr(0, str.length() - 1);

		pos = str.rfind('/');
		string lstr;
		if (pos != string::npos)
			lstr = str.substr(0, pos);
		else
			lstr = "";
		create_dir(lstr);
#ifdef _WIN32
		return CreateDirectoryA(str.c_str(), NULL) ? 0 : -1;
#else
		return mkdir(str.c_str(), 0777);
#endif
	}
	int create_dir_by_filepath(const std::string& filepath)
	{
		//含文件名的全路径
		int pos1, pos2;
		pos1 = (int)filepath.rfind('\\');
		pos2 = (int)filepath.rfind('/');
		if (pos1 == -1 && pos2 == -1)
			return -1;
		if (pos1>pos2)
			return create_dir(filepath.substr(0, pos1));
		else
			return create_dir(filepath.substr(0, pos2));
	}
	int string_array_find(int argc, char** argv, const char* str)
	{
		for (int i = 0; i<argc; ++i)
		{
			if (0 == strcmp(argv[i], str))
				return i;
		}
		return -1;
	}
	int get_scanf_pass(char *pass,int size)
	{
#ifdef _MSC_VER
		//gets(pass);
		int i = 0;
		while (i<size)
		{
			pass[i] = getch();
			if (pass[i] == '\r')
			{
				pass[i] = '\0';
				printf("\n");
				return 0;
			}
			i++;
			printf("*");
		}
#else
		memset(pass, 0, size);
		system("stty -echo");
		//getchar(); 要等回车所以不适用
		fgets(pass,1023,stdin);
		system("stty echo");
		pass[strlen(pass) - 1] = '\0';
		printf("\n");
#endif
		return 0;
	}

	bool big_endian()
	{
		static bool bcheck = false;
		static bool is_big = true;//big_endian
		if (!bcheck)
		{
			unsigned short i = 0x0901;
			if (0x01 == *(char*)&i)
				is_big = false; //little endian
			bcheck = true;
		}
		return is_big;
	}
	char itoc(unsigned int i)
	{
		char c;
		if (big_endian())
			memcpy(&c, ((char*)&i) + 3, 1);
		else
			memcpy(&c, (char*)&i, 1);
		return c;
	}
	std::string& byte2hexs(const uint8_t* from, int fromsize, std::string& s)
	{
		s.resize(fromsize * 2);
		char *to = (char*)s.data();
		for (int i = 0; i < fromsize; ++i)
			snprintf(to + 2 * i, 3, "%02x", from[i]);
		return s;
	}
	std::string& byte2hexs(const std::string& from, std::string& out)
	{
		return byte2hexs((uint8_t*)from.data(), (int)from.size(), out);
	}
	uint8_t* hexs2byte(const std::string& from, uint8_t* to, int tosize)
	{
		unsigned int tmp = 0;
		const char* p = from.data();
		assert(2 * tosize == (int)from.length());
		if (2 * tosize == (int)from.length())
		{
			for (int i = 0; i < tosize; ++i)
			{
				sscanf_s(p + 2 * i, "%2x", &tmp);
				to[i] = itoc(tmp);
			}
		}
		return to;
	}
	std::string& hexs2byte(const std::string& from, std::string& out)
	{
		unsigned int tmp = 0;
		size_t tosize = from.size() / 2;
		const char* p = from.data();

		out.resize(tosize);
		uint8_t* to = (uint8_t*)out.data();
		for (size_t i = 0; i < tosize; ++i)
		{
			sscanf_s(p + 2 * i, "%2x", &tmp);
			to[i] = itoc(tmp);
		}
		return out;
	}
	std::string b2h(const std::string& s)
	{
		string v;
		return byte2hexs(s, v);
	}
	std::string h2b(const std::string& s)
	{
		string v;
		return hexs2byte(s, v);
	}
	std::string randkey(int size)
	{
		std::string k;
		uint64_t tick;
		uint32_t crc32;
		k.resize(size + 4);
		crc32 = CL_CRC32_FIRST;
		srand((unsigned int)time(NULL));
		for (int i = 0; i < size; i += 4)
		{
			tick = cl::utick() + ::rand();
			crc32 = cl::cl_crc32_write(crc32, (uint8_t*)&tick, 8);
			memcpy((char*)k.data() + i, (void*)&crc32, 4);
		}
		k.erase(size);
		return k;
	}

	std::string get_string_index(const std::string& source, int index, const std::string& sp)
	{
		if (sp.empty() || index<0)
			return source;

		int splen = (int)sp.length();
		int pos1 = 0 - splen, pos2 = 0 - splen;

		int i = 0;
		for (i = 0; i<(index + 1); i++)
		{
			pos1 = pos2 + splen;
			pos2 = (int)source.find(sp, pos1);
			if (pos2<0)
				break;
		}
		if (i<index)
			return "";

		pos2 = pos2<pos1 ? (int)source.length() : pos2;
		return source.substr(pos1, pos2 - pos1);

	}
	int get_string_index_pos(const std::string& source, int index, const std::string& sp)
	{
		if (index < 0 || sp.empty())
			return -1;
		int splen = (int)sp.length();
		int pos = 0 - splen;
		for (int i = 0; (pos = (int)source.find(sp, pos + splen)) >= 0 && i<index; i++);
		return pos;
	}
	int get_string_index_count(const std::string& source, const std::string& sp)
	{
		if (sp.empty() || source.empty())
			return 0;
		int i = 1, pos = 0, splen = (int)sp.length();
		pos = 0 - splen;
		for (i = 1; (pos = (int)source.find(sp, pos + splen)) >= 0; i++);
		return i;

	}
	std::string& string_trim(std::string& s)
	{
		if (!s.empty())
		{
			s.erase(0, s.find_first_not_of(" "));
			s.erase(s.find_last_not_of(" ") + 1);
		}
		return s;
	}
	std::string substr_by_findlast(const std::string& s, char _Ch)
	{
		size_t pos = s.find_last_of(_Ch);
		return pos == std::string::npos ? "" : s.substr(pos + 1);
	}

	int atoi(const std::string& _Str, int default_val)
	{
		if (_Str.empty())
			return default_val;
		return ::atoi(_Str.c_str());
	}
	std::string itoa(int i)
	{
		char buf[32];
		sprintf(buf, "%d", i);
		return buf;
	}
	long long atoll(const char* _Str, int nullval/*=0*/)
	{
		long long i = 0;
		if (NULL == _Str)
			return nullval;
		if (1 != sscanf_s(_Str, "%lld", &i))
			return nullval;
		return i;
	}

	//****************************************************
	string time_to_time_string(const std::time_t& _Time)
	{
		std::tm *t = std::localtime(&_Time);
		if (!t)return "";
		char buf[128];
		sprintf(buf, "%02d:%02d:%02d",
			t->tm_hour, t->tm_min, t->tm_sec);
		return buf;
	}
	string time_to_date_string(const std::time_t& _Time)
	{
		std::tm *t = std::localtime(&_Time);
		if (!t)return "";
		char buf[128];
		sprintf(buf, "%d-%02d-%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
		return buf;
	}
	string time_to_datetime_string(const std::time_t& _Time)
	{
		std::tm *t = std::localtime(&_Time);
		if (!t)return "";
		char buf[128];
		sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
		return buf;
	}
	string time_to_datetime_string2(const std::time_t& _Time)
	{
		std::tm *t = std::localtime(&_Time);
		if (!t)return "";
		char buf[128];
		sprintf(buf, "%d%02d%02d.%02d%02d%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
		return buf;
	}
	string time_to_datetimeT_string(const std::time_t& _Time)
	{
		std::tm *t = std::localtime(&_Time);
		if (!t)return "";
		char buf[128];
		sprintf(buf, "%d-%02d-%02dT%02d:%02d:%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
		return buf;
	}

	std::string url_get_param(const std::string& params, const std::string& name)
	{
		size_t pos1=-1, pos2=0;
		string str;
		if (params.empty() || name.empty())
			return "";
		str = name+"=";

		//find pos1
		do {
			pos1 = params.find(str, pos1+1);
			if (pos1 == string::npos)
				return "";
			if (0 == pos1 || '&' == params.at(pos1 - 1) || '?' == params.at(pos1 - 1))
				break;
		} while (1);
		pos1 += str.size();
		//find pos2
		pos2 = params.find('&', pos1);
		if (pos2 > 0) pos2 = pos2 - pos1;
		return params.substr(pos1, pos2);
	}

	int read_file_to_str(const std::string& path, std::string& out)
	{
		long n = 0, ret = 0;
		char *p;
		//打开直接定位到尾部ate
		std::ifstream f(path, std::ios::in|std::ios::binary|std::ios::ate);
		if (f.is_open())
		{
			n = (long)f.tellg();
			f.seekg(0, std::ios::beg);
			out.resize(n);
			p = (char*)out.data();
			while (!f.eof()&&n>0)
			{
				f.read(p, n); 
				ret = (long)f.gcount();
				if (ret <= 0)
					break;
				p += ret;
				n -= ret;
			}
			return n > 0 ? -2 : 0;
		}
		return -1;
	}
	int write_file_from_str(const std::string& path,const std::string& in)
	{
		create_dir_by_filepath(path);
		const char *p = in.data();
		size_t n,ret;
		std::streampos pos;
		n = in.size();
		std::ofstream f(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (f.is_open())
		{
			while (n > 0)
			{
				pos = f.tellp();
				f.write(p, n);
				ret = (size_t)(f.tellp()-pos);
				if (ret <= 0)
					break;
				p += ret;
				n -= ret;
			}
			return n > 0 ? -2 : 0;
		}
		return -1;
	}

	int write_log(const char *strline, const char *path)
	{
		char *buf = new char[strlen(strline) + 128];
		if (!buf)
			return -1;
		time_t tt = time(0);
		tm *t = localtime(&tt);
		sprintf(buf, "[%d-%d-%d %02d:%02d:%02d] %s\r\n",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec, strline);

		FILE *fp = fopen(path, "ab+");
		if (fp)
		{
			fwrite(buf, strlen(buf), 1, fp);
			fclose(fp);
		}
		delete[] buf;
		return 0;
	}
	int write_logt(const char *path, int maxsize, const char* format, ...)
	{
		char *buf = new char[maxsize];
		int size = 0;
		int ret = -1;
		sprintf(buf, "[%s]: ", time_to_datetime_string(std::time(0)).c_str());
		va_list ap;
		va_start(ap, format);
		size = (int)strlen(buf);
		maxsize -= (size + 3); //减掉换行和结束符
		size = vsnprintf(buf + size, maxsize, format, ap);
		va_end(ap);
		if (size>0 && size<maxsize)
		{
			strcat(buf, "\r\n");
			FILE *fp = fopen(path, "ab+");
			if (fp)
			{
				fwrite(buf, strlen(buf), 1, fp);
				fclose(fp);
				ret = 0;
			}
		}
		delete[] buf;
		return ret;
	}
CLEND
