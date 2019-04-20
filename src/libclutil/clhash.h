#pragma once

#include <assert.h>
#include "util.h"
#include "bstream.h"
#include "crc32.h"
#include <time.h>
#include "bit.h"
#include <stdlib.h>
#include <string.h>

namespace cl {

	
	template<int N>
	class hash
	{
	private:
		//std::array<uint8_t, N> m_data;
		uint8_t * m_data;
	public:
		hash() :m_data(new uint8_t[N]) { memset(m_data, 0, N); }
		hash(const std::string& s) : m_data(new uint8_t[N]) { hexs2byte(s, m_data, N); }
		hash(const uint8_t* _data) : m_data(new uint8_t[N]) { memcpy(m_data, _data, N);}
		hash(const hash<N>& _h) : m_data(new uint8_t[N]) { memcpy(m_data, _h.m_data, N);}
		~hash() { delete[] m_data; }
		
		const uint8_t& at(size_t off) const { return m_data[off]; }
		uint8_t* data() const { return m_data; }
		size_t size() const { return N; }
		std::string& to_hexString(std::string& s) {return byte2hexs(m_data, N, s);}
		std::string to_hexString()const { std::string s; return byte2hexs(m_data, N, s); }
		std::string to_string() const { std::string s; s.resize(N); memcpy((char*)s.data(),(char*)m_data,N); return s; }

		hash<N>& operator=(const hash<N>& h) { memcpy(m_data, h.m_data, N); return *this; }
		hash<N>& operator=(const std::string& s) { hexs2byte(s, m_data, N); return *this; }
		hash<N>& operator=(const char* s) { hexs2byte(std::string(s), m_data, N); return *this;}
		hash<N>& operator^=(const hash<N>& h) { for(uint8_t i=0;i<N;++i) m_data[i]^=h.m_data[i]; return *this; }
		hash<N> operator^(const hash<N>& h)const { hash<N> v; for (uint8_t i = 0; i < N; ++i) v.m_data[i] = m_data[i] ^ h.m_data[i]; return v; }

		bool operator==(const hash<N>& h) const { return 0 == strncmp((const char*)m_data, (const char*)h.m_data, N); }
		bool operator!=(const hash<N>& h) const { return 0 != strncmp((const char*)m_data, (const char*)h.m_data, N); }
		bool operator<(const hash<N>& h) const { return 0 > strncmp((const char*)m_data, (const char*)h.m_data, N); }
		hash<N>&  rand() {
			uint32_t crc32 = CL_CRC32_FIRST;
			uint64_t tick;
			srand((unsigned int)time(NULL));
			for (int i = 0; i + 4 <= N; i += 4)
			{
				tick = utick() + ::rand();
				crc32 = cl_crc32_write(crc32, (uint8_t*)&tick, 8);
				memcpy(m_data + i, (void*)&crc32, 4);
			}
			return *this;
		}
		static hash<N> rand_hash() {
			hash<N> h;
			h.rand();
			return h;
		}
	};

	template<int N>
	int hash_distance(const cl::hash<N>& h1, const cl::hash<N>& h2)
	{
		int offset = N * 8;
		unsigned char c;
		uint8_t *p1 = h1.data(), *p2 = h2.data();
		for (int i = 0; i <N; ++i)
		{
			c = p1[i] ^ p2[i];
			offset -= 8;
			if (0 != c)
				return offset + bit::high_flag_table[c];
		}
		return offset;
	}

	using h160 = hash<20>;
	using h256 = hash<32>;
	using h512 = hash<64>;

	template<int N>
	int operator<<(bstream& ss,const hash<N>& h)
	{
		ss.write((const void*)h.data(), N);
		return ss.ok();
	}
	template<int N>
	int operator>>(bstream& ss, hash<N>& h)
	{
		ss.read(h.data(), N);
		return ss.ok();
	}
}

