#include "crypp.h"


#include <algorithm>

#include "cryptlib.h"
#include "osrng.h"	//包含AutoSeededRandomPool算法的头文件
#include "rijndael.h" //AES算法
#include "modes.h"
#include "filters.h"
#include "base64.h"

using namespace CryptoPP;

namespace cl
{

	aescbc::aescbc()
	{
	}


	aescbc::~aescbc()
	{
	}
	std::string aescbc::to_keylen(const std::string& key,size_t len)
	{
		std::string k;
		k.resize(len);
		memset((void*)k.data(), 0, len);
		memcpy((void*)k.data(), (void*)key.data(), std::min(key.size(), len));
		return k;
	}
	std::string aescbc::randkey(size_t len/*=16*/)
	{
		std::string key;
		key.resize(len);
		memset((byte*)key.data(), 0, len);
		//SecByteBlock key(0x00, AES::DEFAULT_KEYLENGTH);
		AutoSeededRandomPool rnd;
		rnd.GenerateBlock((byte*)key.data(), key.size());
		return key;
	}
	/*
	key=iv, 但不够16字的情况，key补0，iv补'0'字符，方面在http://tool.chacuo.net/cryptaes 检验
	*/
	int aescbc::encrypt_s(const std::string& key, const std::string& in, std::string& out)
	{
		if (key.empty() || in.empty()) return -1;
		out.clear();
		try
		{
			byte iv[CryptoPP::AES::BLOCKSIZE];
			memset(iv, '0', CryptoPP::AES::BLOCKSIZE);
			memcpy(iv, key.c_str(), std::min(key.length(),(size_t)AES::BLOCKSIZE));
			AES::Encryption aesEn((byte*)to_keylen(key, AES::DEFAULT_KEYLENGTH).data(), AES::DEFAULT_KEYLENGTH);
			CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEn(aesEn, iv);

			CryptoPP::StreamTransformationFilter stfEn(cbcEn, new StringSink(out));
			stfEn.Put(reinterpret_cast<const unsigned char*>(in.c_str()), in.length());
			stfEn.MessageEnd();
		}
		catch (...)
		{
			return -1;
		}
		return 0;
	}
	int aescbc::decrypt_s(const std::string& key, const std::string& in, std::string& out)
	{
		if (key.empty() || in.empty()) return -1;
		out.clear();
		try
		{
			byte iv[CryptoPP::AES::BLOCKSIZE];
			memset(iv, '0', CryptoPP::AES::BLOCKSIZE);
			memcpy(iv, key.c_str(), std::min(key.length(), (size_t)AES::BLOCKSIZE));
			AES::Decryption aesDe((byte*)to_keylen(key, AES::DEFAULT_KEYLENGTH).data(), AES::DEFAULT_KEYLENGTH);
			CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDe(aesDe, iv);
			//StringSink ss(out);
			CryptoPP::StreamTransformationFilter stfDe(cbcDe, new StringSink(out));
			stfDe.Put(reinterpret_cast<const unsigned char*>(in.c_str()), in.size());
			stfDe.MessageEnd();
		}
		catch (...)
		{
			return -1;
		}
		return 0;
	}
}

