
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern "C"
{
#include "secp256k1.h"
#include "secp256k1_recovery.h"
}

#include "cltype.h"
#include "crc32.h"
#include <time.h>

//#include <openssl/rand.h>
//#ifdef _MSC_VER
//#include <intrin.h>
//#endif

namespace cryp
{
    ///*
    //function:
    //读取CPU使用率（代码来自bitcoin）
    //*/
    //static inline int64_t GetPerformanceCounter()
    //{
    //    // Read the hardware time stamp counter when available.
    //    // See https://en.wikipedia.org/wiki/Time_Stamp_Counter for more information.
    //    #if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    //    return __rdtsc();
    //    #elif !defined(_MSC_VER) && defined(__i386__)
    //    uint64_t r = 0;
    //    __asm__ volatile ("rdtsc" : "=A"(r)); // Constrain the r variable to the eax:edx pair.
    //    return r;
    //    #elif !defined(_MSC_VER) && (defined(__x86_64__) || defined(__amd64__))
    //    uint64_t r1 = 0, r2 = 0;
    //    __asm__ volatile ("rdtsc" : "=a"(r1), "=d"(r2)); // Constrain r1 to rax and r2 to rdx.
    //    return (r2 << 32) | r1;
    //    #else
    //    // Fall back to using C++11 clock (usually microsecond or nanosecond precision)
    //    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    //#endif
    //}
    ///*
    //function:
    //通过CPU使用率生成随机种子（代码来自bitcoin）。
    //*/
    //void RandAddSeed()
    //{
    //    // Seed with CPU performance counter
    //    int64_t nCounter = GetPerformanceCounter();
    //    RAND_add(&nCounter, sizeof(nCounter), 1.5);
    //    memset((void*)&nCounter, 0, sizeof(nCounter));
    //}


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

    /*
    function:
    构建libsecp256k1的context并对其随机化。
    return:
    生成成功返回context。
    */
    secp256k1_context* create_context() 
    {
		unsigned char seed[32];
	    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        //RandAddSeed();
        //RAND_bytes(seed, 32);
		rand_key32(seed);
        secp256k1_context_randomize(ctx, seed);
        return ctx;
    }
	void destroy_context(secp256k1_context* ctx)
	{
		secp256k1_context_destroy(ctx);
	}
    /*
    function:
    随机生成32位的私钥。
    return:
    生成成功返回key。
    */
    unsigned char *generate_key32(const secp256k1_context *ctx,unsigned char key[32]) 
    {
       // unsigned char *key;
       // RandAddSeed();
        do {
            //RAND_bytes(key,32);
			rand_key32(key);
        } while (!secp256k1_ec_seckey_verify(ctx, key));
        return key;
    }
    /*
    function:
    根据私钥生成公钥。
    return:
    生成成功返回key。
    */
    unsigned char * generate_public_key(const secp256k1_context *ctx,unsigned char out_pubkey[66], const unsigned char *key, bool compressed) 
    {
        secp256k1_pubkey tmp_pubkey;
        size_t output_length;
        unsigned int flags;

        if (!compressed)
        {
            output_length = 65;
            flags = SECP256K1_EC_UNCOMPRESSED;
        }
        else
        {
            output_length = 33;
            flags = SECP256K1_EC_COMPRESSED;
        }
        //unsigned char *pubkey = (unsigned char*)malloc(sizeof(unsigned char)*output_length);
        secp256k1_ec_pubkey_create(ctx, &tmp_pubkey, key);
        secp256k1_ec_pubkey_serialize(ctx, out_pubkey, &output_length, &tmp_pubkey, flags);
        return out_pubkey;
    }
    /*
    function:
    根据私钥和消息，生成无须公钥的签章。
    return:
    生成成功返回0，失败返回-1。
    */
    int sign_recoverable(const secp256k1_context *ctx, const unsigned char *msg, const unsigned char *privkey, unsigned char *sig)
    {
        secp256k1_ecdsa_recoverable_signature tmp_sig;
        unsigned char *output64 = (unsigned char*)malloc(sizeof(unsigned char)*64);
        int recid;

        if(!secp256k1_ecdsa_sign_recoverable(ctx, &tmp_sig, msg, privkey, NULL, NULL))
        {
            return -1;
        }
        if(!secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, output64, &recid, &tmp_sig))
        {
            return -1;
        }
        memcpy(sig,output64,64);
        sig[64] = (unsigned char) recid;
        return 0;
    }
    /*
    function:
    根据消息和签章，验证签章。
    return:
    验证成功返回0，失败返回-1。
    */
    int verify_recoverable(const secp256k1_context *ctx, const unsigned char *msg, const unsigned char *sig)
    {
        secp256k1_pubkey tmp_pubkey;
        secp256k1_ecdsa_recoverable_signature tmp_rec_sig;
        secp256k1_ecdsa_signature tmp_sig;

        if(!secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &tmp_rec_sig, sig, int(sig[64])))
        {
            return -1;
        }
        if(!secp256k1_ecdsa_recover(ctx, &tmp_pubkey, &tmp_rec_sig, msg))
        {
            return -1;
        }
        secp256k1_ecdsa_recoverable_signature_convert(ctx, &tmp_sig, &tmp_rec_sig);
        return 1-secp256k1_ecdsa_verify(ctx, &tmp_sig, msg, &tmp_pubkey);
    }
    /*
    function:
    根据私钥和消息，生成需要公钥验证的签章。
    return:
    生成成功返回0，失败返回-1。
    */
    int sign(const secp256k1_context *ctx, const unsigned char *msg, const unsigned char *privkey, unsigned char *sig)
    {
        secp256k1_ecdsa_signature tmp_sig;

        if(!secp256k1_ecdsa_sign(ctx, &tmp_sig, msg, privkey, NULL, NULL))
        {
            return -1;
        }
        if(!secp256k1_ecdsa_signature_serialize_compact(ctx, sig, &tmp_sig))
        {
            return -1;
        }
        return 0;
    }
    /*
    function:
    根据消息、签章和公钥，验证签章。
    return:
    验证成功返回0，失败返回-1。
    */
    int verify(const secp256k1_context *ctx, const unsigned char *msg, const unsigned char *sig, const unsigned char *pubkey)
    {
        secp256k1_ecdsa_signature tmp_sig;
        secp256k1_pubkey tmp_pubkey;

        if(!secp256k1_ecdsa_signature_parse_compact(ctx, &tmp_sig, sig))
        {
            return -1;
        }
        if(!secp256k1_ec_pubkey_parse(ctx, &tmp_pubkey, pubkey, strlen((char *)pubkey)))
        {
            return -1;
        }
        return 1-secp256k1_ecdsa_verify(ctx, &tmp_sig, msg, &tmp_pubkey);
    }
}