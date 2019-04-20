#pragma once
#include "singleton.h"
#include "cltype.h"



class setting
{
public:
	setting();
	~setting();

public:
	int load();

protected:
	GETSET(uint16_t, m_port, _port);
	GETSET(std::string, m_dbaddr, _dbaddr);
};


typedef cl::singleton<setting> settingSngl;

