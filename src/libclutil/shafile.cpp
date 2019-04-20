#include "shafile.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "file64.h"
#include "cltype.h"

namespace cl
{

	int sha1_file(unsigned char* res20, const char* sFile, int iSleepMSec/*=-1*/, bool pmsg/*=true*/)
	{
		if (pmsg) printf("sha1_file: path=%s \n", sFile);

		int i = 0, j = 0;
		SHA1_CONTEXT ctx;
		file64 file;

		if (0 != file.open(sFile, F64_READ))
			return -1;
		size64_t size = file.seek(0, SEEK_END);
		file.seek(0, SEEK_SET);
		if (pmsg) printf("#hash size=%lld \n ", size);

		//init
		sha1_init(&ctx);
		unsigned char *buffer = new unsigned char[16384];
		j = 0;
		size64_t readSize = 0;
		while (readSize < size)
		{
			if (0 >= (i = file.read((char*)buffer, 16384)))
			{
				perror("#:read() failed:");
				break;
			}
			//buid
			sha1_write(&ctx, buffer, i);
			readSize += i;
			j++;
			if (iSleepMSec >= 0 && 0 == j % 16)
			{
				msleep(iSleepMSec);
			}

			if (pmsg && 0 == j % 10)
				printf("\r hash_per = %d (%lld/%lld)", (int)(readSize / (size / 100 + 1)), readSize, size);
		}
		delete[] buffer;
		//fini
		sha1_final(&ctx);
		//未算完整文件当失败
		assert(readSize == (size64_t)file.tell());
		assert(readSize == size);
		if (readSize < size)
			return -1;
		file.close();

		memcpy(res20, ctx.buf, 20);
		return 0;
	}

}


