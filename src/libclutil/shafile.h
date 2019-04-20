#pragma once

#include "sha1.h"
#include "clsha3.h"

namespace cl
{

	//**************************
	//以下是简单包装运算
	//res20长度必须满足20BIT， iSleepMSec表示每运算一长度时休息一下
	int sha1_file(unsigned char* res20, const char* sFile, int iSleepMSec = -1, bool pmsg = true);

}