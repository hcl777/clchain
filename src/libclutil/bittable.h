#pragma once
#include "bstream.h"

namespace cl {

	/**************
	��λ������λ.
	bitsizeָλ��.
	setsizeָ��1�ĸ���.
	�Զ�����λֵΪ1�ĸ���.
	**************/
	class bittable
	{
	public:
		bittable(void);
		~bittable(void);

		void alloc(int bitsize, const unsigned char* vbuf = 0); //�����vbuf,�Ϳ�
		void free();

		const unsigned char* buffer() const { return m_buf; }
		int get_bitsize()const { return m_bitsize; }
		int get_setsize()const { return m_setsize; }
		bool is_setall() const { return (m_bitsize != 0 && m_setsize == m_bitsize); }

		void set(int i, bool v = true);
		void setall(bool v = true);
		bool get(int i) const;
		bool operator[](unsigned int i) const { return get(i); }
		const bittable& operator=(const bittable& bt);

		unsigned char* buffer() { return m_buf; }

		//test:
		static void print(bittable& bt);
		static void test();
	private:
		unsigned char* m_buf;
		int m_bitsize; //
		int m_setsize; //���Ϊ1�ĸ���
	};

	int operator << (bstream& ss, const bittable& inf);
	int operator >> (bstream& ss, bittable& inf);


}
