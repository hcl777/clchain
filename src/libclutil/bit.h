#pragma once
#include <string>

namespace cl {
	namespace bit {

		//计算二进制整数含1的个数
		int mm_popcnt_u8(unsigned char n);
		int mm_popcnt_u32(unsigned int n);
		int mm_popcnt_ub(const unsigned char *buf, int bitsize);

		//计算最高位为1的位置
		extern const unsigned char high_flag_table[256];
		int hight_flag_index(unsigned int n);
		//计算最高位为1的位置：从高字节向低字节数，字节内从低位向高位数
		int hight_flag_index(unsigned char* buf,int size);
		std::string& to_string(unsigned char* buf, int size, std::string& s);

		void bitprint(unsigned char* buf, int size);
	}
}


