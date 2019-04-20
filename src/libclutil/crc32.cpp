#include "crc32.h"


namespace cl
{
	static unsigned int cl_crc32_table[256];
	static bool cl_crc32_table_init = false;
	void cl_crc32_init()
	{
		int i, j;
		for (i = 0; i < 256; i++)
			for (j = 0, cl_crc32_table[i] = i; j < 8; j++)
				cl_crc32_table[i] = (cl_crc32_table[i] >> 1) ^ ((cl_crc32_table[i] & 1) ? 0xEDB88320 : 0);
		cl_crc32_table_init = true;
	}
	unsigned int cl_crc32_write(unsigned int crc32, const unsigned char* buf, int len)
	{
		if (!cl_crc32_table_init)
			cl_crc32_init();
		for (int i = 0; i < len; i++)
		{
			crc32 = cl_crc32_table[((crc32 & 0xFF) ^ buf[i])] ^ (crc32 >> 8);
		}
		crc32 = ~crc32;
		return crc32;
	}

	// init 的替代方法
	//	  int i,j;
	//	  unsigned int n;
	//   for(i = 0;i < 256;i++)
	//   {
	//       n = i;
	//       for(j = 0;j < 8;j++)
	//       {
	//           if(n & 1)
	//               n = (n >> 1) ^ 0xEDB88320;
	//           else
	//               n = n >> 1;
	//       }
	//        cl_crc32_table[i] = n;
	//   }

}
