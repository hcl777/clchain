#include "secp.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include "secp256k1.h"
#include "secp256k1_recovery.h"
#ifdef __cplusplus
}
#endif

#include "crc32.h"
#include <time.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <chrono>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace cryp
{
	uint64_t get_cpurate()
	{
		// Read the hardware time stamp counter when available.
		// See https://en.wikipedia.org/wiki/Time_Stamp_Counter for more information.
		#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
		return __rdtsc();
		#elif !defined(_MSC_VER) && defined(__i386__)
		uint64_t r = 0;
		__asm__ volatile ("rdtsc" : "=A"(r)); // Constrain the r variable to the eax:edx pair.
		return r;
		#elif !defined(_MSC_VER) && (defined(__x86_64__) || defined(__amd64__))
		uint64_t r1 = 0, r2 = 0;
		__asm__ volatile ("rdtsc" : "=a"(r1), "=d"(r2)); // Constrain r1 to rax and r2 to rdx.
		return (r2 << 32) | r1;
		#else
		// Fall back to using C++11 clock (usually microsecond or nanosecond precision)
		return std::chrono::high_resolution_clock::now().time_since_epoch().count();
		#endif
	}
	uint8_t* rand_key32(uint8_t buf[32])
	{
		uint32_t crc32 = CL_CRC32_FIRST;
		uint64_t tick;
		srand((unsigned int)time(NULL));
		for (int i = 0; i + 4 <= 32; i += 4)
		{
			tick = cl::utick() + ::rand();
			crc32 = cl::cl_crc32_write(crc32, (uint8_t*)&tick, 8);
			memcpy(buf + i, (void*)&crc32, 4);
		}
		return buf;
	}

	//创建ctx比较耗时间，PC约100ms,可多线程使用
	secp256k1_context* secp_create_context()
	{
		unsigned char seed[32];
		rand_key32(seed);
		secp256k1_context *ctx;
		ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
		if (ctx)
		{
			if (1!=secp256k1_context_randomize(ctx, seed))
			{
				secp256k1_context_destroy(ctx);
				ctx = NULL;
			}
		}
			
		return ctx;
	}
	void secp_destroy_context(secp256k1_context* ctx)
	{
		secp256k1_context_destroy(ctx);
	}

	//****************************************************
	static secp256k1_context *ctx = NULL;
	int secp_init()
	{
		if (NULL == ctx)
			ctx = secp_create_context();
		return 0;
	}
	void secp_fini()
	{
		if (ctx)
		{
			secp_destroy_context(ctx);
			ctx = NULL;
		}
	}

	//必须用key32进行签章(即prikey)
	int secp_generate_key(std::string& prikey32, std::string& pubkey)
	{
		secp_init();
		int ret = -1;

		//随机生成KEY1（在此用短的作公钥）
		prikey32.resize(32);
		uint8_t* key1 = (uint8_t*)prikey32.data();
		do {
			rand_key32(key1);
		} while (!secp256k1_ec_seckey_verify(ctx, key1));

		//生成KEY2，非压缩
		//SECP256K1_EC_UNCOMPRESSED，SECP256K1_EC_COMPRESSED
		secp256k1_pubkey secpkey;
		uint8_t key2[65];
		uint32_t flag = SECP256K1_EC_COMPRESSED;
		size_t outlen = (flag==SECP256K1_EC_UNCOMPRESSED?65:33);

		while (1 != secp256k1_ec_pubkey_create(ctx, &secpkey, key1));
		if (1 == secp256k1_ec_pubkey_serialize(ctx, key2, &outlen, &secpkey, flag))
		{
			if (outlen > 0)
			{
				pubkey.resize(outlen);
				memcpy((char*)pubkey.data(), key2, outlen);
				ret = 0;;
			}
		}
		//test: 测试重用ctx与不重用
		//secp_fini();
		return ret;
	}

	//int secp_sign_nopubkey_check(const std::string& key32, uint8_t msg32[32], std::string& out64)
	//{
	//	secp_init();
	//	int ret = -1;
	//	secp256k1_ecdsa_recoverable_signature tmp_sig;
	//	int recid = 0;
	//	out64.resize(64);
	//	//参数msg32必须32长，key必须是32长那个KEY
	//	if (secp256k1_ecdsa_sign_recoverable(ctx, &tmp_sig, msg32, (uint8_t*)key32.data(), NULL, NULL))
	//	{
	//		if (secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx,(uint8_t*)out64.data(),&recid,&tmp_sig))
	//			ret = 0;
	//	}
	//	//secp_fini();
	//	return ret;
	//}

	int secp_sign(const std::string& prikey32,const uint8_t msg32[32], std::string& out64)
	{
		secp_init();
		int ret = -1;
		secp256k1_ecdsa_signature tmp_sig;
		out64.resize(64);
		//参数msg32必须32长，key必须是32长那个KEY
		if (secp256k1_ecdsa_sign(ctx, &tmp_sig, msg32, (uint8_t*)prikey32.data(), NULL, NULL))
		{
			if (secp256k1_ecdsa_signature_serialize_compact(ctx, (uint8_t*)out64.data(), &tmp_sig))
				ret = 0;
		}
		//secp_fini();
		return ret;
	}
	int secp_verify_sign(const std::string& pubkey, const std::string& sig64, const uint8_t msg32[32])
	{
		secp_init();
		int ret = -1;

		secp256k1_ecdsa_signature tmp_sig;
		secp256k1_pubkey tmp_pubkey;
		if (secp256k1_ec_pubkey_parse(ctx, &tmp_pubkey, (const uint8_t*)pubkey.data(), pubkey.size()))
		{
			if (secp256k1_ecdsa_signature_parse_compact(ctx, &tmp_sig, (const uint8_t*)sig64.data()))
			{
				if (secp256k1_ecdsa_verify(ctx, &tmp_sig, msg32, &tmp_pubkey))
					ret = 0;
			}
		}

		//secp_fini();
		return ret;
	}
}



