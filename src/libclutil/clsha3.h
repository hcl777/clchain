#pragma once

#include <stdint.h>
#include <stdlib.h>

/*
�������ԣ�����������ٶȵͣ���sha1��10��ʱ�䡣����10MB������Ҫ1�롣
*/
namespace cl
{
#define decsha3(bits) \
	int sha3_##bits(uint8_t* out, size_t outlen, uint8_t const* in, size_t inlen);

	decsha3(256)
	decsha3(512)

}


