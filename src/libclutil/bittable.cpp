#include "bittable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bit.h"

namespace cl {


	bittable::bittable(void)
		:m_buf(NULL)
		, m_bitsize(0)
		, m_setsize(0)
	{
	}
	bittable::~bittable(void)
	{
		free();
	}
	void bittable::alloc(int bitsize, const unsigned char* vbuf/*=0*/)
	{
		free();
		assert(bitsize>0);
		if (bitsize <= 0) return;
		m_bitsize = bitsize;
		m_buf = new unsigned char[(m_bitsize + 7) >> 3];
		if (0 == vbuf)
		{
			memset(m_buf, 0, (m_bitsize + 7) >> 3);
		}
		else
		{
			memcpy(m_buf, vbuf, (m_bitsize + 7) >> 3);
			m_setsize = bit::mm_popcnt_ub(m_buf, m_bitsize);
		}
	}
	void bittable::free()
	{
		if (m_buf)
		{
			delete[] m_buf;
			m_buf = NULL;
			m_bitsize = 0;
			m_setsize = 0;
		}
	}
	void bittable::set(int i, bool v/*=true*/)
	{
		assert(i >= 0 && i<m_bitsize);
		if (i >= 0 && i<m_bitsize)
		{
			int index = i >> 3;
			int offset = i & 0x07;
			unsigned char mask = (unsigned char)0x01 << offset;
			if (v)
			{
				if ((m_buf[index] & mask) == 0)
				{
					m_buf[index] |= mask;
					m_setsize++;
				}
			}
			else
			{
				if ((m_buf[index] & mask) != 0)
				{
					m_buf[index] &= ~mask;
					m_setsize--;
				}
			}
		}
	}
	void bittable::setall(bool v/*=true*/)
	{
		int n = v ? 0xffffffff : 0;
		if (m_buf)
		{
			memset(m_buf, n, (m_bitsize + 7) >> 3);
			if (v)
				m_setsize = m_bitsize;
			else
				m_setsize = 0;
		}
	}
	bool bittable::get(int i) const
	{
		assert(i >= 0 && i<m_bitsize);
		if (i >= 0 && i<m_bitsize)
		{
			int index = i >> 3;
			int offset = i & 0x07;
			unsigned char mask = (unsigned char)0x01 << offset;
			return (m_buf[index] & mask) != 0;
		}
		return false;
	}
	const bittable& bittable::operator=(const bittable& bt)
	{
		alloc(bt.get_bitsize(), bt.buffer());
		return *this;
	}
	
	int operator << (bstream& ss, const bittable& inf)
	{
		assert(inf.get_setsize() == bit::mm_popcnt_ub(inf.buffer(), inf.get_bitsize()));
		ss << inf.get_bitsize();
		if (inf.get_bitsize()>0)
			ss.write(inf.buffer(), ((inf.get_bitsize() + 7) >> 3));
		return ss.ok();
	}
	int operator >> (bstream& ss, bittable& inf)
	{
		unsigned int bitsize = 0;
		ss >> bitsize;
		if (bitsize>0)
		{
			inf.alloc(bitsize, (const unsigned char*)ss.read_ptr());
			ss.skipr((bitsize + 7) >> 3);
		}
		return ss.ok();
	}

	//*********************************************************************
	//test:
	void bittable::print(bittable& bt)
	{
		printf("#bt[%d / %d]:", bt.get_setsize(), bt.get_bitsize());
		for (int i = 0; i<bt.get_bitsize(); ++i)
		{
			if (0 == i % 8)printf(" ");
			printf("%d", bt[i] ? 1 : 0);
		}
		printf("\n");
	}
	
	void bittable::test()
	{
		//bool b = false;
		bittable bt;
		bt.alloc(50);

		bt.setall();
		printf("bit,set=%d:", bit::mm_popcnt_ub(bt.buffer(), bt.get_bitsize()));
		for (int i = 0; i<bt.get_bitsize(); ++i)
		{
			if (0 == i % 8)printf(" ");
			printf("%d", bt[i] ? 1 : 0);
		}
		printf("\n");

		bt.set(17, false);
		bt.set(27, false);
		bt.set(32, false);
		bt.set(39, false);
		printf("bit,set=%d:", bit::mm_popcnt_ub(bt.buffer(), bt.get_bitsize()));
		for (int i = 0; i<bt.get_bitsize(); ++i)
		{
			if (0 == i % 8)printf(" ");
			printf("%d", bt[i] ? 1 : 0);
		}
		printf("\n");


		bt.set(12, false);
		bt.set(46, false);
		bt.set(48, false);
		printf("bit,set=%d:", bit::mm_popcnt_ub(bt.buffer(), bt.get_bitsize()));
		for (int i = 0; i<bt.get_bitsize(); ++i)
		{
			if (0 == i % 8)printf(" ");
			printf("%d", bt[i] ? 1 : 0);
		}
		printf("\n");
		//b = false;
	}
}
