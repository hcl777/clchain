#pragma once
#include "singleton.h"
#include "block.h"
#include "guards.h"

#include "sqlite3.h"


/*
暂仅用于记录交易日志
*/

class secdb
{
	
	//tranflow: type,uid,desid,height,traidx,asset,balance,time
public:
	secdb();
	~secdb();
	int init(const std::string& path);
	void fini();
	int reset_db();//当put genesis重置

	int save_tranlog(const transaction_cert_t& tc, uint64_t h, int traidx);
	int save_flowi(const string& id, const flow_i_t& f);
	int save_block_flow(block_t& b);
	int get_flow(string& sret, const string& uid, const string& beg_date, const string& end_date);
private:
	void close_db();
	int create_table(sqlite3* db);
private:
	typedef std::mutex Mutex;
	typedef std::lock_guard<Mutex> Lock;

	Mutex		m_mt;
	sqlite3*	m_db;
	string		m_path;
};

typedef cl::singleton<secdb> secdbSngl;
//
//"'id' integer primary key autoincrement,"
//"'type' integer not null, "
//"'uid' nvarchar(132) not null, "
//"'desid' nvarchar(132) not null,"
//"'height' integer not null, "
//"'traidx' integer not null, "
//"'asset' integer not null, "
//"'balance' integer not null,"
//"'time' datetime)"

