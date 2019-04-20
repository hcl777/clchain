#include "bit.h"
#include <cstring>

namespace cl {
	namespace bit {

		int mm_popcnt_u8(unsigned char n)
		{
			n = (n & 0x55) + ((n >> 1) & 0x55);
			n = (n & 0x33) + ((n >> 2) & 0x33);
			n = (n & 0x0f) + ((n >> 4) & 0x0f);
			return n;
		}
		int mm_popcnt_u32(unsigned int n)
		{
			n = (n & 0x55555555) + ((n >> 1) & 0x55555555); //相邻1位和,2位保存
			n = (n & 0x33333333) + ((n >> 2) & 0x33333333); //相邻2位和,4位保存
			n = (n & 0x0f0f0f0f) + ((n >> 4) & 0x0f0f0f0f); //相邻4位和,8位保存
			n = (n & 0x00ff00ff) + ((n >> 8) & 0x00ff00ff); //相邻8位和,16位保存
			n = (n & 0x0000ffff) + ((n >> 16) & 0x0000ffff); //相邻16位和,32位保存 
			return n;
		}
		int mm_popcnt_ub(const unsigned char *buf, int bitsize)
		{
			int i, n, m, j, k;
			char c;
			n = bitsize >> 3;
			m = n & 0x03;
			j = bitsize & 0x7;
			k = 0;
			for (i = 0; i + 3<n; i += 4, buf += 4)
				k += mm_popcnt_u32(*(unsigned int*)buf);
			for (i = 0; i<m; ++i, ++buf)
				k += mm_popcnt_u8(*buf);
			if (j>0)
			{
				c = *buf;
				for (i = 0; i<j; ++i, c >>= 1)
					k += (c & 1);
			}
			return k;
		}

		const unsigned char high_flag_table[256] = {
			0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
			5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
			6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
			6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
		};
		int hight_flag_index(unsigned int n)
		{
			if (0 == n)return 0;
			int offset = 24;
			while ((n >> offset) == 0)
				offset -= 8;
			return high_flag_table[n >> offset] + offset;
		}
		int hight_flag_index(unsigned char* buf, int size)
		{
			int offset = size*8;
			for (int i = 0; i <size; ++i)
			{
				offset -= 8;
				if (0 != buf[i])
					return offset + high_flag_table[buf[i]];
			}
			return offset;
		}
		std::string& to_string(unsigned char* buf, int size, std::string& s)
		{
			s.resize(size * 9);
			char* p = (char*)s.data();
			char c;
			for (int i = 0; i < size; ++i)
			{
				c = buf[i];
				snprintf(p + i * 9, 10, "%1d%1d%1d%1d%1d%1d%1d%1d "
					, (int)(c >> 7 & 0x1)
					, (int)(c >> 6 & 0x1)
					, (int)(c >> 5 & 0x1)
					, (int)(c >> 4 & 0x1)
					, (int)(c >> 3 & 0x1)
					, (int)(c >> 2 & 0x1)
					, (int)(c >> 1 & 0x1)
					, (int)(c  & 0x1)
				);
			}
			return s;
		}
		void bitprint(unsigned char* buf, int size)
		{
			char *s = new char[size * 9 + 1];
			memset(s, 0, size * 9 + 1);
			int j = 0,k=0;
			unsigned char c;
			for (int i = 0; i<size; ++i)
			{
				c = buf[i];
				for(k=7;k>=0;k--)
					s[j++] = ((c>>k)&0x1) ?'1':'0';
				s[j++] = ' ';
			}
			printf("%s\n",s);
			delete[] s;
		}
	}
}
