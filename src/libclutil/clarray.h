#pragma once
#include <string.h>
#include <assert.h>
namespace cl
{
#define BASEARRAY2_SUBLEN 1024

	template<class T>
	class array
	{

	private:
		T * *data;
		char *state;
		int size;
		int count;
		int cursor;
	public:
		array() :data(0), state(0), size(0), count(0), cursor(0) { }
		~array() { resize(0); }

		int resize(int asize)
		{
			//reset
			if (size)
			{
				int len = (size - 1) / BASEARRAY2_SUBLEN + 1;
				for (int i = 0; i < len; ++i)
					delete[] data[i];
				delete[] data;
				data = 0;
				delete[] state;
				state = 0;
				size = 0;
				count = 0;
				cursor = 0;
			}

			//new
			if (asize)
			{
				size = asize;
				int len = (size - 1) / BASEARRAY2_SUBLEN + 1;
				data = new T*[len];
				for (int i = 0; i < len; ++i)
					data[i] = new T[BASEARRAY2_SUBLEN];
				len = (size + 7) >> 3;
				state = new char[len];
				memset(state, 0, len);
			}
			return 0;
		}

		int rallot()
		{
			if (count >= size)
				return -1;
			int n = -1, i = 0;
			for (i = cursor; i < size; ++i)
			{
				if (0 == (state[i >> 3] & (1 << (i & 0x07))))
				{
					n = i;
					break;
				}
			}
			if (-1 == n)
			{
				for (i = 0; i < cursor; ++i)
				{
					if (0 == (state[i >> 3] & (1 << (i & 0x07))))
					{
						n = i;
						break;
					}
				}
			}
			if (-1 == n)
			{
				//assert(0);
				return -1;
			}

			count++;
			state[n >> 3] |= 1 << (n & 0x07);
			cursor = n + 1;
			if (cursor >= size)
				cursor = 0;
			return n;
		}

		void free(int i)
		{
			assert(i < size);
			state[i >> 3] &= ~(1 << (i & 0x07));
			count--;
		}
		T& operator [](int i)
		{
			assert(i < size);
			return data[i / BASEARRAY2_SUBLEN][i%BASEARRAY2_SUBLEN];
		}
	};

}
