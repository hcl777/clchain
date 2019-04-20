#include "setting.h"



setting::setting()
{
	m_port = 5080;
	m_dbaddr = "hcl:111@192.168.5.12:3306/chain";
}


setting::~setting()
{
}
int setting::load()
{
	return 0;
}

