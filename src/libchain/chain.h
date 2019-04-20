#pragma once
#include "cpo.h"
#include "blockcache.h"
#include "singleton.h"
#include "speaker.h"
#include "guards.h"
/*
chain 保存数据：
1。连ID
2。最新块信息高度和hash
3。最新仲裁会员名单
4。区块链： (key=height(1~n)，val=block)
5.utxo资产: (key=id+"A"，val=asset)
6。地址表：(key=id,val=pubkey)
*/

class chainListener
{
public:
	virtual ~chainListener() {}
	template<int I>
	struct S {enum{T=I}; };

	typedef S<1> UpdateDirectors;
	typedef S<2> UpdateReferee;
	typedef S<3> UpdateLBI;

	virtual void on(UpdateDirectors) {}
	virtual void on(UpdateReferee) {}
	virtual void on(UpdateLBI) {}
};

class chain : public speaker<chainListener>
{
public:
	chain();
	~chain();

public:
	int init(const std::string& dbpath,uint32_t cid); //这个cid仅在链未存在时有用
	void fini();
	int genesis(const std::string& jpath);
	uint32_t cid()const { return m_set.cid; }
	string cids()const { return cl::itoa(m_set.cid); }
	int find_tran(const hash_t& tranhash, uint64_t in_height);//-1不存在，0未知，1存在
	int check_tran_ok(transaction_t& t);
	int check_block_ok(block_t& b, bool bcheck_votesign = true); //会生成hash
	int put_block(char *buf,int size,bool btry_cache=true);
	int put_block(block_t& b);
	
	int rollback_to(uint64_t height); //回滚至此高度height
	const chain_setting_t& get_set() const { return m_set; }
	uint8_t get_in_ex()const { return m_set.in_ex; }
	uint64_t get_last_height()const { return m_lbi.bt.height; }
	int get_last_blockinfo(cpo_last_t& lbi) { Lock l(m_mt); lbi = m_lbi; return 0; }
	bool get_block(uint64_t i, string& s);
	int get_referees(map<userid_t, int>& v) { Lock l(m_mt); v = m_refereemp; return 0; }

	tran_names_t& get_directors(tran_names_t& v) { Lock l(m_mt); v = m_directors; return v; }
	tran_names_t& get_referees(tran_names_t& v) { Lock l(m_mt); v = m_referees; return v; }
	uint64_t get_directors_height()const { return m_directors.height; }
	uint64_t get_referees_height()const { return m_referees.height; }

	size_t min_vote_num()const { return (m_referees.names.size() - 1) * 2 / 3 + 1; }

	int find_utxo_assets(const string& id, assets_t& as);
	static int make_uas(vector<user_assets_t>& uas, map<string, int64_t>& asmp, uint64_t height);
	static int make_uas2(vector<user_assets_t>& uas, map<string, flows_t>& mp);

	int get_sync_task(uint64_t h, list<uint64_t>& ls);
	bool exist_token(const string& tokid);
private:
	int put_genesis_block(block_t& b,bool check_cid=true);
	void update_refereemp();
	//static string find_key_from_map(const userid_t& id, map<userid_t, string>& mp);
	//string find_authorizer_key(const userid_t& id);
	//string find_key(const userid_t& id);
	//string find_transaction_key(transaction_cert_t& t);
	bool is_director(const userid_t& id);
	void try_put_block_from_cache();


	//static void tokenkey_add(string& keys, const string& k);
	int cache_tranhash(uint64_t h, vector<hash_t>& v);
	int get_tranhash_from_db(uint64_t h, vector<hash_t>& v);
	flows_t& find_flows(block_t& b,const string& id);
private:
	typedef std::recursive_mutex CMutex;
	typedef std::lock_guard<CMutex> Lock;

	CMutex					m_mt;
	chain_setting_t			m_set;
	cpo_last_t				m_lbi;
	tran_names_t				m_directors; //第1个为主
	tran_names_t				m_referees; //
	map<userid_t, int>		m_refereemp; //方便高效查询
	//string					m_stokenkeys;
	list<string>			m_tokenkeys;
	//uint64_t				m_min_tranfee,m_min_consfee; //转账和部署合约手续费
	leveldb::DB*			m_db;
	leveldb::WriteOptions	m_wop;
	blockcache				m_cache;
	map<uint64_t, map<hash_t, char>>	m_tranhashs;
	
};

typedef cl::singleton<chain> chainSngl;

