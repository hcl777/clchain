#include "aes128.h"
#include "claes.h"
#include "util.h"
#include "clmd5.h"

namespace cl
{

	//*********************************
	std::string aes128::randkey()
	{
		return cl::randkey(16);
	}
	std::string aes128::to_key128(const std::string& key)
	{
		std::string k;
		k.resize(16);
		memset((void*)k.data(), 0, 16);
		memcpy((void*)k.data(), (void*)key.data(), std::min((int)key.size(), 16));
		return k;
	}
#define DATA_POS 33
	int aes128::encrypt_s(const std::string& key, const std::string& in, std::string& out)
	{
		//16B(key_md5)+16B(data_md5)+1B(多余数量)+AESBUF
		aes a;
		size_t i;
		char buf[16];
		char c = 0;
		u1byte *p;
		if (key.empty() || in.empty()) return -1;
		a.set_key((u1byte*)to_key128(key).c_str(), 128);
		out.resize((in.size() + 15) / 16 * 16 + DATA_POS);
		memset(buf, 0, 16);
		p = (u1byte*)out.data() + DATA_POS;
		//保存key和原文的MD5值用于解码较对，确保解密信息正确完整
		md5::md5_buffer(key.c_str(), key.size(), (uint8_t*)out.data());
		md5::md5_buffer(in.c_str(), in.size(), (uint8_t*)out.data()+16);
		for (i = 0; i+16 < in.size(); i += 16)
			a.encrypt((u1byte*)in.data() + i, p+i);

		if (i < in.size())
		{
			c = (char)(i+16-in.size());
			memcpy(buf, in.data() + i, 16 - c);
			a.encrypt((u1byte*)buf, p + i);
			i += 16;
		}
		out.at(DATA_POS-1) = c; //最后字节记录有多少个多余字符
		return 0;
	}
	int aes128::decrypt_s(const std::string& key, const std::string& in, std::string& out)
	{
		aes a;
		size_t i;
		char buf[16];
		if (key.empty() || in.size()<DATA_POS+16||in.size()%16!=1 || in.at(DATA_POS-1)>16) return -1;
		//检查密钥是否正确
		md5::md5_buffer(key.c_str(), key.size(), (uint8_t*)buf);
		if (0 != memcmp(buf, in.data(), 16))
			return -1;
		a.set_key((u1byte*)to_key128(key).c_str(), 128);
		out.resize(in.size()- DATA_POS);
		u1byte* p = (u1byte*)in.data() + DATA_POS;
		for (i = 0; i + 15 < out.size(); i += 16)
			a.decrypt(p + i, (u1byte*)out.data() + i);
		if(out.size()>(size_t)in.at(DATA_POS - 1))
			out.erase(out.size() - in.at(DATA_POS - 1));
		//检查原文是否完整正确
		md5::md5_buffer(out.c_str(), out.size(), (uint8_t*)buf);
		if (0 != memcmp(buf, in.data()+16, 16))
			return -2;
		return 0;
	}
}

