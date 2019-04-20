
/* sha1sum.c - print SHA-1 cl_Message-Digest Algorithm
* Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
* Copyright (C) 2004 g10 Code GmbH
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2, or (at your option) any
* later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* SHA-1 coden take from gnupg 1.3.92.

Note, that this is a simple tool to be used for MS Windows.
*/

#include "sha1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#undef BIG_ENDIAN_HOST

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
#include <windows.h>
#pragma warning(disable:4996)
#define msleep Sleep
#else
#include <unistd.h>
#define msleep(dwMilliseconds) usleep(dwMilliseconds*1000)
#endif //WIN32

namespace cl
{

	/****************
	* Rotate a 32 bit integer by n bytes
	****************/
	//经linux测试,使用__asm__比直接移位慢1/2
	//#if defined(__GNUC__) && defined(__i386__)
	//static inline u32 rol( u32 x, int n)
	//{
	//	__asm__("roll %%cl,%0"
	//		:"=r" (x)
	//		:"0" (x),"c" (n));
	//	return x;
	//}
	//#else
	//#define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
	//#endif
	//
#define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )

	void sha1_init(SHA1_CONTEXT *hd)
	{
		hd->h0 = 0x67452301;
		hd->h1 = 0xefcdab89;
		hd->h2 = 0x98badcfe;
		hd->h3 = 0x10325476;
		hd->h4 = 0xc3d2e1f0;
		hd->nblocks = 0;
		hd->count = 0;
	}

	/*
	* Transform the message X which consists of 16 32-bit-words
	*/
	void transform(SHA1_CONTEXT *hd, const unsigned char *data)
	{
		u32 a, b, c, d, e, tm;
		u32 *x = hd->x;

		/* get values from the chaining vars */
		a = hd->h0;
		b = hd->h1;
		c = hd->h2;
		d = hd->h3;
		e = hd->h4;

#ifdef BIG_ENDIAN_HOST
		memcpy(x, data, 64);
#else
		unsigned char *p2 = (unsigned char *)x;
		p2[3] = data[0];
		p2[2] = data[1];
		p2[1] = data[2];
		p2[0] = data[3];
		p2[7] = data[4];
		p2[6] = data[5];
		p2[5] = data[6];
		p2[4] = data[7];
		p2[11] = data[8];
		p2[10] = data[9];
		p2[9] = data[10];
		p2[8] = data[11];
		p2[15] = data[12];
		p2[14] = data[13];
		p2[13] = data[14];
		p2[12] = data[15];
		p2[19] = data[16];
		p2[18] = data[17];
		p2[17] = data[18];
		p2[16] = data[19];
		p2[23] = data[20];
		p2[22] = data[21];
		p2[21] = data[22];
		p2[20] = data[23];
		p2[27] = data[24];
		p2[26] = data[25];
		p2[25] = data[26];
		p2[24] = data[27];
		p2[31] = data[28];
		p2[30] = data[29];
		p2[29] = data[30];
		p2[28] = data[31];
		p2[35] = data[32];
		p2[34] = data[33];
		p2[33] = data[34];
		p2[32] = data[35];
		p2[39] = data[36];
		p2[38] = data[37];
		p2[37] = data[38];
		p2[36] = data[39];
		p2[43] = data[40];
		p2[42] = data[41];
		p2[41] = data[42];
		p2[40] = data[43];
		p2[47] = data[44];
		p2[46] = data[45];
		p2[45] = data[46];
		p2[44] = data[47];
		p2[51] = data[48];
		p2[50] = data[49];
		p2[49] = data[50];
		p2[48] = data[51];
		p2[55] = data[52];
		p2[54] = data[53];
		p2[53] = data[54];
		p2[52] = data[55];
		p2[59] = data[56];
		p2[58] = data[57];
		p2[57] = data[58];
		p2[56] = data[59];
		p2[63] = data[60];
		p2[62] = data[61];
		p2[61] = data[62];
		p2[60] = data[63];

		//去掉循环提高速度
		//int i;
		//unsigned char *p2;
		//for(i=0, p2=(unsigned char*)x; i < 16; i++, p2 += 4 ) 
		//{
		//	p2[3] = *data++;
		//	p2[2] = *data++;
		//	p2[1] = *data++;
		//	p2[0] = *data++;
		//}
#endif

#define K1  0x5A827999L
#define K2  0x6ED9EBA1L
#define K3  0x8F1BBCDCL
#define K4  0xCA62C1D6L
#define F1(x,y,z)   ( z ^ ( x & ( y ^ z ) ) )
#define F2(x,y,z)   ( x ^ y ^ z )
#define F3(x,y,z)   ( ( x & y ) | ( z & ( x | y ) ) )
#define F4(x,y,z)   ( x ^ y ^ z )

#define R(a,b,c,d,e,f,k,m)  do { e += rol( a, 5 )   \
		+ f( b, c, d )                          \
		+ k                                     \
		+ m;                                    \
		b = rol( b, 30 );                           \
	} while(0)


		R(a, b, c, d, e, F1, K1, x[0]);
		R(e, a, b, c, d, F1, K1, x[1]);
		R(d, e, a, b, c, F1, K1, x[2]);
		R(c, d, e, a, b, F1, K1, x[3]);
		R(b, c, d, e, a, F1, K1, x[4]);
		R(a, b, c, d, e, F1, K1, x[5]);
		R(e, a, b, c, d, F1, K1, x[6]);
		R(d, e, a, b, c, F1, K1, x[7]);
		R(c, d, e, a, b, F1, K1, x[8]);
		R(b, c, d, e, a, F1, K1, x[9]);
		R(a, b, c, d, e, F1, K1, x[10]);
		R(e, a, b, c, d, F1, K1, x[11]);
		R(d, e, a, b, c, F1, K1, x[12]);
		R(c, d, e, a, b, F1, K1, x[13]);
		R(b, c, d, e, a, F1, K1, x[14]);
		R(a, b, c, d, e, F1, K1, x[15]);

		//#define M(i) ( tm =   x[i&0x0f] ^ x[(i-14)&0x0f] ^ x[(i-8)&0x0f] ^ x[(i-3)&0x0f] , (x[i&0x0f] = rol(tm,1)) )
		//R( e, a, b, c, d, F1, K1, M(16) );
		//R( d, e, a, b, c, F1, K1, M(17) );
		//R( c, d, e, a, b, F1, K1, M(18) );
		//R( b, c, d, e, a, F1, K1, M(19) );
		//R( a, b, c, d, e, F2, K2, M(20) );
		//R( e, a, b, c, d, F2, K2, M(21) );
		//R( d, e, a, b, c, F2, K2, M(22) );
		//R( c, d, e, a, b, F2, K2, M(23) );
		//R( b, c, d, e, a, F2, K2, M(24) );
		//R( a, b, c, d, e, F2, K2, M(25) );
		//R( e, a, b, c, d, F2, K2, M(26) );
		//R( d, e, a, b, c, F2, K2, M(27) );
		//R( c, d, e, a, b, F2, K2, M(28) );
		//R( b, c, d, e, a, F2, K2, M(29) );
		//R( a, b, c, d, e, F2, K2, M(30) );
		//R( e, a, b, c, d, F2, K2, M(31) );
		//R( d, e, a, b, c, F2, K2, M(32) );
		//R( c, d, e, a, b, F2, K2, M(33) );
		//R( b, c, d, e, a, F2, K2, M(34) );
		//R( a, b, c, d, e, F2, K2, M(35) );
		//R( e, a, b, c, d, F2, K2, M(36) );
		//R( d, e, a, b, c, F2, K2, M(37) );
		//R( c, d, e, a, b, F2, K2, M(38) );
		//R( b, c, d, e, a, F2, K2, M(39) );
		//R( a, b, c, d, e, F3, K3, M(40) );
		//R( e, a, b, c, d, F3, K3, M(41) );
		//R( d, e, a, b, c, F3, K3, M(42) );
		//R( c, d, e, a, b, F3, K3, M(43) );
		//R( b, c, d, e, a, F3, K3, M(44) );
		//R( a, b, c, d, e, F3, K3, M(45) );
		//R( e, a, b, c, d, F3, K3, M(46) );
		//R( d, e, a, b, c, F3, K3, M(47) );
		//R( c, d, e, a, b, F3, K3, M(48) );
		//R( b, c, d, e, a, F3, K3, M(49) );
		//R( a, b, c, d, e, F3, K3, M(50) );
		//R( e, a, b, c, d, F3, K3, M(51) );
		//R( d, e, a, b, c, F3, K3, M(52) );
		//R( c, d, e, a, b, F3, K3, M(53) );
		//R( b, c, d, e, a, F3, K3, M(54) );
		//R( a, b, c, d, e, F3, K3, M(55) );
		//R( e, a, b, c, d, F3, K3, M(56) );
		//R( d, e, a, b, c, F3, K3, M(57) );
		//R( c, d, e, a, b, F3, K3, M(58) );
		//R( b, c, d, e, a, F3, K3, M(59) );
		//R( a, b, c, d, e, F4, K4, M(60) );
		//R( e, a, b, c, d, F4, K4, M(61) );
		//R( d, e, a, b, c, F4, K4, M(62) );
		//R( c, d, e, a, b, F4, K4, M(63) );
		//R( b, c, d, e, a, F4, K4, M(64) );
		//R( a, b, c, d, e, F4, K4, M(65) );
		//R( e, a, b, c, d, F4, K4, M(66) );
		//R( d, e, a, b, c, F4, K4, M(67) );
		//R( c, d, e, a, b, F4, K4, M(68) );
		//R( b, c, d, e, a, F4, K4, M(69) );
		//R( a, b, c, d, e, F4, K4, M(70) );
		//R( e, a, b, c, d, F4, K4, M(71) );
		//R( d, e, a, b, c, F4, K4, M(72) );
		//R( c, d, e, a, b, F4, K4, M(73) );
		//R( b, c, d, e, a, F4, K4, M(74) );
		//R( a, b, c, d, e, F4, K4, M(75) );
		//R( e, a, b, c, d, F4, K4, M(76) );
		//R( d, e, a, b, c, F4, K4, M(77) );
		//R( c, d, e, a, b, F4, K4, M(78) );
		//R( b, c, d, e, a, F4, K4, M(79) );

		tm = 0;
#define M(i1,i2,i3,i4) tm=x[i1] ^ x[i2] ^ x[i3] ^ x[i4]; x[i1]=rol(tm,1);
		//#define M(i1,i2,i3,i4) ( tm =  x[i1] ^ x[i2] ^ x[i3] ^ x[i4] , (x[i1] = rol(tm,1)) )
		M(0, 2, 8, 13); R(e, a, b, c, d, F1, K1, x[0]);
		M(1, 3, 9, 14); R(d, e, a, b, c, F1, K1, x[1]);
		M(2, 4, 10, 15); R(c, d, e, a, b, F1, K1, x[2]);
		M(3, 5, 11, 0); R(b, c, d, e, a, F1, K1, x[3]);
		M(4, 6, 12, 1); R(a, b, c, d, e, F2, K2, x[4]);
		M(5, 7, 13, 2); R(e, a, b, c, d, F2, K2, x[5]);
		M(6, 8, 14, 3); R(d, e, a, b, c, F2, K2, x[6]);
		M(7, 9, 15, 4); R(c, d, e, a, b, F2, K2, x[7]);
		M(8, 10, 0, 5); R(b, c, d, e, a, F2, K2, x[8]);
		M(9, 11, 1, 6); R(a, b, c, d, e, F2, K2, x[9]);
		M(10, 12, 2, 7); R(e, a, b, c, d, F2, K2, x[10]);
		M(11, 13, 3, 8); R(d, e, a, b, c, F2, K2, x[11]);
		M(12, 14, 4, 9); R(c, d, e, a, b, F2, K2, x[12]);
		M(13, 15, 5, 10); R(b, c, d, e, a, F2, K2, x[13]);
		M(14, 0, 6, 11); R(a, b, c, d, e, F2, K2, x[14]);
		M(15, 1, 7, 12); R(e, a, b, c, d, F2, K2, x[15]);

		M(0, 2, 8, 13); R(d, e, a, b, c, F2, K2, x[0]);
		M(1, 3, 9, 14); R(c, d, e, a, b, F2, K2, x[1]);
		M(2, 4, 10, 15); R(b, c, d, e, a, F2, K2, x[2]);
		M(3, 5, 11, 0); R(a, b, c, d, e, F2, K2, x[3]);
		M(4, 6, 12, 1); R(e, a, b, c, d, F2, K2, x[4]);
		M(5, 7, 13, 2); R(d, e, a, b, c, F2, K2, x[5]);
		M(6, 8, 14, 3); R(c, d, e, a, b, F2, K2, x[6]);
		M(7, 9, 15, 4); R(b, c, d, e, a, F2, K2, x[7]);
		M(8, 10, 0, 5); R(a, b, c, d, e, F3, K3, x[8]);
		M(9, 11, 1, 6); R(e, a, b, c, d, F3, K3, x[9]);
		M(10, 12, 2, 7); R(d, e, a, b, c, F3, K3, x[10]);
		M(11, 13, 3, 8); R(c, d, e, a, b, F3, K3, x[11]);
		M(12, 14, 4, 9); R(b, c, d, e, a, F3, K3, x[12]);
		M(13, 15, 5, 10); R(a, b, c, d, e, F3, K3, x[13]);
		M(14, 0, 6, 11); R(e, a, b, c, d, F3, K3, x[14]);
		M(15, 1, 7, 12); R(d, e, a, b, c, F3, K3, x[15]);

		M(0, 2, 8, 13); R(c, d, e, a, b, F3, K3, x[0]);
		M(1, 3, 9, 14); R(b, c, d, e, a, F3, K3, x[1]);
		M(2, 4, 10, 15); R(a, b, c, d, e, F3, K3, x[2]);
		M(3, 5, 11, 0); R(e, a, b, c, d, F3, K3, x[3]);
		M(4, 6, 12, 1); R(d, e, a, b, c, F3, K3, x[4]);
		M(5, 7, 13, 2); R(c, d, e, a, b, F3, K3, x[5]);
		M(6, 8, 14, 3); R(b, c, d, e, a, F3, K3, x[6]);
		M(7, 9, 15, 4); R(a, b, c, d, e, F3, K3, x[7]);
		M(8, 10, 0, 5); R(e, a, b, c, d, F3, K3, x[8]);
		M(9, 11, 1, 6); R(d, e, a, b, c, F3, K3, x[9]);
		M(10, 12, 2, 7); R(c, d, e, a, b, F3, K3, x[10]);
		M(11, 13, 3, 8); R(b, c, d, e, a, F3, K3, x[11]);
		M(12, 14, 4, 9); R(a, b, c, d, e, F4, K4, x[12]);
		M(13, 15, 5, 10); R(e, a, b, c, d, F4, K4, x[13]);
		M(14, 0, 6, 11); R(d, e, a, b, c, F4, K4, x[14]);
		M(15, 1, 7, 12); R(c, d, e, a, b, F4, K4, x[15]);

		M(0, 2, 8, 13); R(b, c, d, e, a, F4, K4, x[0]);
		M(1, 3, 9, 14); R(a, b, c, d, e, F4, K4, x[1]);
		M(2, 4, 10, 15); R(e, a, b, c, d, F4, K4, x[2]);
		M(3, 5, 11, 0); R(d, e, a, b, c, F4, K4, x[3]);
		M(4, 6, 12, 1); R(c, d, e, a, b, F4, K4, x[4]);
		M(5, 7, 13, 2); R(b, c, d, e, a, F4, K4, x[5]);
		M(6, 8, 14, 3); R(a, b, c, d, e, F4, K4, x[6]);
		M(7, 9, 15, 4); R(e, a, b, c, d, F4, K4, x[7]);
		M(8, 10, 0, 5); R(d, e, a, b, c, F4, K4, x[8]);
		M(9, 11, 1, 6); R(c, d, e, a, b, F4, K4, x[9]);
		M(10, 12, 2, 7); R(b, c, d, e, a, F4, K4, x[10]);
		M(11, 13, 3, 8); R(a, b, c, d, e, F4, K4, x[11]);
		M(12, 14, 4, 9); R(e, a, b, c, d, F4, K4, x[12]);
		M(13, 15, 5, 10); R(d, e, a, b, c, F4, K4, x[13]);
		M(14, 0, 6, 11); R(c, d, e, a, b, F4, K4, x[14]);
		M(15, 1, 7, 12); R(b, c, d, e, a, F4, K4, x[15]);

		/* Update chaining vars */
		hd->h0 += a;
		hd->h1 += b;
		hd->h2 += c;
		hd->h3 += d;
		hd->h4 += e;
	}


	/* Update the message digest with the contents
	* of INBUF with length INLEN.
	*/
	void sha1_write(SHA1_CONTEXT *hd, const unsigned char *inbuf, int inlen)
	{
		int n = 0;
		//有可能外部补完数据
		if (hd->count == 64)
		{ /* flush the buffer */
			transform(hd, hd->buf);
			hd->count = 0;
			hd->nblocks++;
		}
		if (!inbuf) return;
		if (hd->count)
		{
			//有数据
			n = 64 - hd->count;
			if (n > inlen) n = inlen;
			memcpy(hd->buf + hd->count, inbuf, n);
			inbuf += n;
			inlen -= n;
			if (hd->count == 64)
			{ /* flush the buffer */
				transform(hd, hd->buf);
				hd->count = 0;
				hd->nblocks++;
			}
		}

		hd->nblocks += (inlen >> 6);

		//int m = inlen & 0x003f;
		//int size = inlen & 0xffffffc0;
		//n = 0;
		//while(n<size)
		//{
		//	transform( hd, &inbuf[n] );
		//	n += 64;
		//}
		//if(m)
		//{
		//	memcpy(hd->buf,inbuf+n,m);
		//	hd->count = m;
		//}

		//while(inlen>255)
		//{
		//	transform( hd, inbuf );
		//	transform( hd, inbuf+64 );
		//	transform( hd, inbuf+128 );
		//	transform( hd, inbuf+192 );
		//	inbuf += 256;
		//	inlen -= 256;
		//}
		while (inlen > 63) {
			transform(hd, inbuf);
			inlen -= 64;
			inbuf += 64;
		}
		if (inlen)
		{
			memcpy(hd->buf, inbuf, inlen);
			hd->count = inlen;
		}
	}


	/* The routine final terminates the computation and
	* returns the digest.
	* The handle is prepared for a new cycle, but adding bytes to the
	* handle will the destroy the returned buffer.
	* Returns: 20 bytes representing the digest.
	*/

	void sha1_final(SHA1_CONTEXT *hd)
	{
		u32 t, msb, lsb;
		unsigned char *p;
		assert(hd->count < 64);

		t = hd->nblocks;
		/* multiply by 64 to make a byte count */
		lsb = t << 6;
		msb = t >> 26;
		/* add the count */
		t = lsb;
		if ((lsb += hd->count) < t)
			msb++;
		/* multiply by 8 to make a bit count */
		t = lsb;
		lsb <<= 3;
		msb <<= 3;
		msb |= t >> 29;

		if (hd->count < 56) { /* enough room */
			hd->buf[hd->count++] = 0x80; /* pad */
			while (hd->count < 56)
				hd->buf[hd->count++] = 0;  /* pad */
		}
		else { /* need one extra block */
			hd->buf[hd->count++] = 0x80; /* pad character */
			while (hd->count < 64)
				hd->buf[hd->count++] = 0;
			sha1_write(hd, NULL, 0);  /* flush */;
			memset(hd->buf, 0, 56); /* fill next block with zeroes */
		}
		/* append the 64 bit count */
		hd->buf[56] = msb >> 24;
		hd->buf[57] = msb >> 16;
		hd->buf[58] = msb >> 8;
		hd->buf[59] = msb;
		hd->buf[60] = lsb >> 24;
		hd->buf[61] = lsb >> 16;
		hd->buf[62] = lsb >> 8;
		hd->buf[63] = lsb;
		transform(hd, hd->buf);

		p = hd->buf;
#ifdef BIG_ENDIAN_HOST
#define X(a) do { *(u32*)p = hd->h##a ; p += 4; } while(0)
#else /* little endian */
#define X(a) do { *p++ = hd->h##a >> 24; *p++ = hd->h##a >> 16; \
	*p++ = hd->h##a >> 8; *p++ = hd->h##a; } while(0)
#endif
		X(0);
		X(1);
		X(2);
		X(3);
		X(4);
#undef X
	}


	//int Sha1_BuildBuffer(const char* buf, int bufLen, char* sOutStr, char* sOutBuf)
	int sha1_buffer(unsigned char* res, int reslen, unsigned char const* buf, int buflen)
	{
		if (reslen != 20 || !buf || buflen <= 0)
			return -1;
		SHA1_CONTEXT ctx;
		sha1_init(&ctx);
		sha1_write(&ctx, buf, buflen);
		sha1_final(&ctx);
		memcpy(res, ctx.buf, 20);
		return 0;
	}

	//*****************************
}

