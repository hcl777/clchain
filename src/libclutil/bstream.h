#pragma once
#include <assert.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include "slice.h"

namespace cl
{
	typedef unsigned char	uchar;
	typedef int16_t			int16;
	typedef uint16_t		uint16;
	typedef int32_t			int32;
	typedef uint32_t		uint32;
	typedef int64_t			int64;
	typedef uint64_t		uint64;

	enum { SS_LITTLE_ENDIAN = 0, SS_BIG_ENDIAN = 1, SS_HOST_ENDIAN = 2 };
	class bstream
	{
	public:
		bstream(char endian = SS_LITTLE_ENDIAN);
		bstream(int buflen, char endian = SS_LITTLE_ENDIAN);
		bstream(char* buf, int buflen, int datalen = 0, char endian = SS_LITTLE_ENDIAN);
		virtual ~bstream();
	public:
		static bool little_endian();
		static void swap(void *arr, int len);

		//h:host endian; b:big endian; l:little endian ;
		static void htol(void *arr, int len);
		static void ltoh(void *arr, int len);
		static void htob(void *arr, int len);
		static void btoh(void *arr, int len);

		static short htol16(short val);
		static short ltoh16(short val);
		static short htob16(short val);
		static short btoh16(short val);

		static int32 htol32(int32 val);
		static int32 ltoh32(int32 val);
		static int32 htob32(int32 val);
		static int32 btoh32(int32 val);

		static int64 htol64(int64 val);
		static int64 ltoh64(int64 val);
		static int64 htob64(int64 val);
		static int64 btoh64(int64 val);

	public:
		int ok() const { return m_state; }
		char* buffer() const { return m_buf; }
		char* read_ptr() const { return m_buf + m_rpos; }
		char* write_ptr() const { return m_buf + m_wpos; }
		int buffer_size() const { return m_size; }
		int length() const { return m_wpos - m_rpos; }
		int tellr() const { return m_rpos; }
		int tellw() const { return m_wpos; }
		void zero_rw() { m_rpos = m_wpos = 0; }
		int seekr(int pos);
		int seekw(int pos);
		int skipr(int len);
		int skipw(int len);
		int read(void *arr, int len);
		int write(const void *arr, int len);
		int set_memery(int pos, void *arr, int len);
		int get_memery(int pos, void *arr, int len);

		template<typename T>
		int read_array(T *arr, int len)
		{
			for (int i = 0; i<len; ++i)
				(*this) >> arr[i];
			return m_state;
		}

		template<typename T>
		int write_array(T *arr, int len)
		{
			for (int i = 0; i<len; ++i)
				(*this) << arr[i];
			return m_state;
		}

		int read_string(char* str, int maxsize);
		int write_string(const char* str);

		int operator >> (char& val);
		int operator >> (uchar& val);
		int operator >> (int16& val);
		int operator >> (uint16& val);
		int operator >> (int32& val);
		int operator >> (uint32& val);
		int operator >> (int64& val);
		int operator >> (uint64& val);
		//浮点数，假设大多数CPU架构都是IEEE754 标准，不考虑转换。没有字节顺的说法
		//float = 32位，double=64 位
		int operator >> (float& val);
		int operator >> (double& val);

		int operator << (char val);
		int operator << (uchar val);
		int operator << (int16 val);
		int operator << (uint16 val);
		int operator << (int32 val);
		int operator << (uint32 val);
		int operator << (int64 val);
		int operator << (uint64 val);
		int operator << (float val);
		int operator << (double val);

	protected:
		void htomy(void *arr, int len);
		void mytoh(void *arr, int len);
		int check_resize(int more);

	public:
		int attach(char* buf, int buflen, int datalen = 0);
		void reset();
		int fitsize32(int pos)
		{
			//pos位置打上32位包大小
			assert(m_wpos >= (pos + 4));
			if (m_wpos >= (pos + 4))
			{
				int tmp = m_wpos;
				htomy(&tmp, 4);
				set_memery(pos, &tmp, 4);
				return 0;
			}
			m_state = -1;
			return m_state;
		}
	protected:
		char m_endian;
		char* m_buf;
		int m_size;
		int m_state;
		int m_rpos;
		int m_wpos;
		bool m_mynew;
	};

	//string 与 slice的序列化可交叉兼容
	int operator << (bstream& ss, const std::string& s);
	int operator >> (bstream& ss, std::string& s);
	int operator << (bstream& ss, const clslice& s);
	int operator >> (bstream& ss, clslice& s);
	

	//list
	template<typename T>
	int operator << (bstream& ss, const std::list<T>& i)
	{
		ss << (uint32_t)i.size();
		for (typename std::list<T>::const_iterator it = i.begin(); it != i.end(); ++it)
			ss << (*it);
		return ss.ok();
	}
	template<typename T>
	int operator >> (bstream& ss, std::list<T>& i)
	{
		uint32_t size = 0;
		T t;
		if (0 == ss >> size)
		{
			i.clear();
			for (uint32_t j = 0; j < size; ++j)
			{
				if (0 == ss >> t)
					i.push_back(t);
			}
				
		}
		return ss.ok();
	}
	//map
	template<typename K,typename V>
	int operator << (bstream& ss, const std::map<K,V>& i)
	{
		ss << (uint32_t)i.size();
		for (typename std::map<K, V>::const_iterator it = i.begin(); it != i.end(); ++it)
		{
			ss << it->first;
			ss << it->second;
		}
		return ss.ok();
	}
	/*
	GCC 4.8 has a bug 57824): multiline raw strings cannot be the arguments to macros. 
	Don't use multiline raw strings directly in macros with this compiler.
	map<>使用string 如果是二进制数据，有换行的情况下，4.8版本编译运行时会出错。map[]不成功。
	下面函数出现过错误，2个元素最后只有一个元素。不是必然错误。所以最好不要使用多行raw字符做键。
	*/
	template<typename K, typename V>
	int operator >> (bstream& ss, std::map<K, V>& i)
	{
		uint32_t n = 0;
		K k;
		V v;
		if (0 == ss >> n)
		{
			i.clear();
			for (uint32_t j = 0; j < n; ++j)
			{
				ss >> k;
				ss >> v;
				if (0 == ss.ok())
					i[k] = v;	
			}

		}
		return ss.ok();
	}

	//vector
	template<typename T>
	int operator << (bstream& ss, const std::vector<T>& i)
	{
		uint32_t size = (uint32_t)i.size();
		ss << size;
		for (uint32_t j = 0; j < size; ++j)
			ss << i[j];
		return ss.ok();
	}
	template<typename T>
	int operator >> (bstream& ss, std::vector<T>& i)
	{
		uint32_t size = 0;
		if (0 == ss >> size)
		{
			i.resize(size);
			for (uint32_t j = 0; j < size; ++j)
				ss >> i[j];

		}
		return ss.ok();
	}

	//string
	template<typename T>
	std::string& struct_to_string(T& t,std::string& out, int refer_size, char endian = SS_LITTLE_ENDIAN)
	{
		out.resize(refer_size);
		bstream b((char*)out.data(), (int)out.size(), 0, endian);
		b << t;
		out.erase(b.length());
		assert(0==b.ok());
		return out;
	}

	template<typename T>
	int string_to_struct(const std::string& s, T& out, char endian = SS_LITTLE_ENDIAN)
	{
		if (s.empty()) return -1;
		bstream b((char*)s.data(), (int)s.size(), (int)s.size(), endian);
		b >> out;
		return b.ok();
	}

	
}

