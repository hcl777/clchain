#pragma once

namespace cl
{

	//enum KeySize { Bits128, Bits192, Bits256 };  // key size, in bits, for construtor
	typedef unsigned char   u1byte; /* an 8 bit unsigned character type */
	typedef unsigned short  u2byte; /* a 16 bit unsigned integer type   */
	typedef unsigned int	u4byte; /* a 32 bit unsigned integer type   */
	typedef signed char     s1byte; /* an 8 bit signed character type   */
	typedef signed short    s2byte; /* a 16 bit signed integer type     */
	typedef signed int		s4byte; /* a 32 bit signed integer type     */
									/* 2. Standard interface for AES cryptographic routines             */
									/* These are all based on 32 bit unsigned values and may require    */
									/* endian conversion for big-endian architectures                   */


	/* 3. Basic macros for speeding up generic operations               */
	/* Circular rotate of 32 bit values                                 */


	//AES是美国国家标准技术研究所NIST旨在取代DES的21世纪的加密标准
	//应用于大数据量加密如文件，加密后大小一致

	/********************************
	使用说明:
	set_key(key,key_len=key的位数).
	如key = 32字节的长度时, key_len = 256.
	key长度支持16byte=128,24byte,32byte=256..
	加密/解密 时每16字节为1个单元.
	*********************************/
	class aes
	{
	public:
		aes() {}
		virtual ~aes() {}
	public:
		void set_key(const u1byte key[], const u4byte key_len);
		void encrypt(const u1byte in_blk[16], u1byte out_blk[16]);
		void decrypt(const u1byte in_blk[16], u1byte out_blk[16]);

		//余数部分不加密,直接将数据加到原来buf
		void encrypt_n(u1byte iobuf[], int size);
		void decrypt_n(u1byte iobuf[], int size);

		//随机生成key,一般为16B或者32B.set时要转为bit.
		static int rand_key(u1byte key[], int size);
	private:
		u4byte  k_len;
		u4byte  e_key[64];
		u4byte  d_key[64];

	};

}

