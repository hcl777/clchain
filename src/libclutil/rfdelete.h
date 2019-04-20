#pragma once

namespace cl
{
	//删除文件，如果是一个目录也会删除,unlink(),rmdir()
	int rfdelete(const char* path);
}

