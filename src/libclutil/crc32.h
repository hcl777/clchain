#pragma once

namespace cl
{
#define CL_CRC32_FIRST 0xFFFFFFFF
	unsigned int cl_crc32_write(unsigned int crc32, const unsigned char* buf, int len);
}


