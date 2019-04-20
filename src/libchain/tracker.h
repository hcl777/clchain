#pragma once
#include <singleton.h>
#include "trackproto.h"
#include <map>
#include <vector>
#include <leveldb/db.h>
#include <guards.h>

/*
功能：
	（通过HTTPSVR 服务，作为nginx的upstream服务器。
	作为接入点程序，不操作账本，实现仲裁会地址管理，授权者地址管理。授权者不一定需要账本。
	1。接收来自仲裁会员的报告议会地址。
	2。接收来自授权者的地址报告。	
	3。提供外部查询仲裁会地址
	4。增加mysql相关统计，监测链状态。
	5。后期考虑这功能迁至WEB实现更多其它功能。
		
*/


class tracker
{
public:
	tracker();
	~tracker();

public:
	static int genesis_conf(const genesis_conf_t& gc);
	static bool gc_ok(const genesis_conf_t& gc);
	
	int init();
	void fini();
	uint32_t get_cid()const { return m_gc.cid; }
	int get_rand_node(nodeaddr_t& n);

	int on_update_directors(const std::string& s);
	int on_update_referees(const std::string& s);
	int on_report_refstate(const std::string& s);
	int search_referees(map<userid_t, nodeaddr_t>& mp);
	int search_addr(const userid_t& id,nodeaddr_t& na);
	int get_state(track_state_t& s);
private:
	template<typename T>
	bool ldbv(leveldb::DB* db, leveldb::ReadOptions& rp, const string& k, T& v)
	{
		string s;
		return (db->Get(rp, k, &s).ok() && 0 == cl::string_to_struct(s, v));
	}
	const tp_state_t* find_tnodei(const userid_t& id)const;
	bool legalid(const userid_t& id);
	int update_namelist(tran_names_t& src_ns, tran_names_t& ns, map<userid_t, tp_state_t>& mp,const string& dbk);
private:
	Mutex								m_mt;
	leveldb::DB*						m_db;
	leveldb::WriteOptions				m_wop;
	uint64_t							m_begintime;
	genesis_conf_t						m_gc;
	tran_names_t							m_direcs;
	tran_names_t							m_refers;
	map<userid_t, tp_state_t>			m_directors;
	map<userid_t, tp_state_t>			m_referees;
};

typedef cl::singleton<tracker> trackerSngl;

