#pragma once

/*
win: ����10MBԼ100ms��
*/
namespace cl
{
	//**************************
	//160 bit

	typedef unsigned int u32;
	typedef struct {
		u32  h0, h1, h2, h3, h4;
		u32  nblocks;
		unsigned char buf[64];
		int  count;
		u32 x[16]; //��ʱ������
	} SHA1_CONTEXT;

	void sha1_init(SHA1_CONTEXT *hd);
	void sha1_write(SHA1_CONTEXT *hd, const unsigned char *inbuf, int inlen);
	void sha1_final(SHA1_CONTEXT *hd);
	//**************************


	int sha1_buffer(unsigned char* res, int reslen, unsigned char const* buf, int buflen);

}

