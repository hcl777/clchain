#pragma once

#include "sha1.h"
#include "clsha3.h"

namespace cl
{

	//**************************
	//�����Ǽ򵥰�װ����
	//res20���ȱ�������20BIT�� iSleepMSec��ʾÿ����һ����ʱ��Ϣһ��
	int sha1_file(unsigned char* res20, const char* sFile, int iSleepMSec = -1, bool pmsg = true);

}