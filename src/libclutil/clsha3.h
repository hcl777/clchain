#pragma once

#include <stdint.h>
#include <stdlib.h>

/*
经过测试，本代码计算速度低，是sha1的10倍时间。计算10MB数据需要1秒。
*/
namespace cl
{
#define decsha3(bits) \
	int sha3_##bits(uint8_t* out, size_t outlen, uint8_t const* in, size_t inlen);

	decsha3(256)
	decsha3(512)

}


