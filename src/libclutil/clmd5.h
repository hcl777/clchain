#pragma once
#include <stdlib.h>

namespace cl
{
	namespace md5
	{
		typedef struct
		{
			unsigned int count[2];
			unsigned int state[4];
			unsigned char buffer[64];

		}CL_MD5_CTX;

		void md5_init(CL_MD5_CTX *context);
		void md5_update(CL_MD5_CTX *context,const unsigned char *input, unsigned int inputlen);
		void md5_final(CL_MD5_CTX *context, unsigned char digest[16]);

		unsigned char* md5_buffer(const char* buf, size_t size, unsigned char out[16]);
	}
}

