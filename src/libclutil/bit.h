#pragma once
#include <string>

namespace cl {
	namespace bit {

		//���������������1�ĸ���
		int mm_popcnt_u8(unsigned char n);
		int mm_popcnt_u32(unsigned int n);
		int mm_popcnt_ub(const unsigned char *buf, int bitsize);

		//�������λΪ1��λ��
		extern const unsigned char high_flag_table[256];
		int hight_flag_index(unsigned int n);
		//�������λΪ1��λ�ã��Ӹ��ֽ�����ֽ������ֽ��ڴӵ�λ���λ��
		int hight_flag_index(unsigned char* buf,int size);
		std::string& to_string(unsigned char* buf, int size, std::string& s);

		void bitprint(unsigned char* buf, int size);
	}
}


