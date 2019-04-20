#include "tracker.h"
#include "bstream.h"
#include "signature.h"
#include "clsha3.h"
#include "cpo.h"
#include "util.h"



//*********************************************************

#define DBK_GC			"genesis_conf"


tracker::tracker()
	:m_db(NULL)
{
	m_gc.cid = 0;
	m_begintime = time(NULL);
	m_direcs.height = 0;
	m_refers.height = 0;
}

tracker::~tracker()
{
}
int tracker::genesis_conf(const genesis_conf_t& gc)
{
	cl::create_dir("tdata");
	string path = cl::get_module_dir() + "tdata/db";
	int ret = 0;
	leveldb::DB* db;
	leveldb::Options options;
	leveldb::Status s;
	leveldb::ReadOptions rp;
	leveldb::WriteOptions	wp;
	wp.sync = true;
	if (!gc_ok(gc)) return 1;
	options.error_if_exists = true;
	options.create_if_missing = true;

	s = leveldb::DB::Open(options, path, &db);
	if (!s.ok())
		return 2;
	string v;
	if (db->Get(rp, DBK_GC, &v).ok())
		ret = 3;
	else if (!db->Put(wp, DBK_GC, struct_to_string(gc, v, 1024)).ok())
		ret = 4;
	delete db;
	return ret;
}
bool tracker::gc_ok(const genesis_conf_t& gc)
{
	if (gc.cid == 0 || gc.genesisid.empty())
		return false;
	return true;
}
int tracker::init()
{
	if (m_db)
		return 1;

	string path = cl::get_module_dir() + "tdata/db";
	leveldb::Options options;
	leveldb::Status s;
	leveldb::ReadOptions rp;
	m_wop.sync = true;
	s = leveldb::DB::Open(options, path, &m_db);
	if (!s.ok())
	{
			return -1;
	}
	ldbv(m_db,rp, DBK_GC, m_gc);
	if (!gc_ok(m_gc))
	{
		printf("*** no genesis config!\n");
		fini();
		return -2;
	}
	ldbv(m_db, rp, DBK_DIRECTORS, m_direcs);
	ldbv(m_db, rp, DBK_REFEREES, m_refers);
	size_t i;
	tp_state_t ni,ni2;
	for (i=0;i<m_direcs.names.size();++i)
	{
		ni2.na.id = m_direcs.names[i];
		m_directors[m_direcs.names[i]] = ldbv(m_db, rp, m_direcs.names[i], ni) ? ni : ni2;
	}
	for (i = 0; i<m_refers.names.size(); ++i)
	{
		ni2.na.id = m_refers.names[i];
		m_referees[m_refers.names[i]] = ldbv(m_db, rp, m_refers.names[i], ni) ? ni : ni2;
	}

	return 0;
}

void tracker::fini()
{
	if (m_db)
	{
		delete m_db;
		m_db = NULL;
	}
}
const tp_state_t* tracker::find_tnodei(const userid_t& id)const
{
	map<userid_t, tp_state_t>::const_iterator it;
	it = m_directors.find(id);
	if (it != m_directors.end())
		return &(it->second);
	it = m_referees.find(id);
	if (it != m_referees.end())
		return &it->second;
	return NULL;
}
/*
检查是否为白名单
*/
bool tracker::legalid(const userid_t& id)
{
	if (id == m_gc.genesisid)
		return true;
	if (m_directors.find(id) != m_directors.end())
		return true;
	if (m_referees.find(id) != m_referees.end())
		return true;
	return false;
}
int tracker::update_namelist(tran_names_t& src_ns, tran_names_t& ns, map<userid_t, tp_state_t>& mp, const string& dbk)
{
	map<userid_t, tp_state_t> p;
	map<userid_t, tp_state_t>::iterator it;
	tp_state_t ni;
	size_t i;
	string str;
	//创建新表
	for (i = 0; i < src_ns.names.size(); ++i)
	{
		//printf("%s\n", cl::byte2hexs(src_ns.names[i], str).c_str());
		ni.na.id = src_ns.names[i];
		it = mp.find(src_ns.names[i]);
		if (it != mp.end())
			p[src_ns.names[i]] = it->second;
		else
			p[src_ns.names[i]] = ni;
	}
	ns.names.swap(src_ns.names);
	ns.height = src_ns.height;
	//ns.timestamp = time(NULL);
	mp.swap(p);
	//清除旧节点
	for (i = 0; i < src_ns.names.size(); ++i)
	{
		if (!legalid(src_ns.names[i]))
		{
			m_db->Delete(m_wop, src_ns.names[i]);
		}
	}
	m_db->Put(m_wop, dbk, cl::struct_to_string(ns, str, 4096));
	return 0;
}
#define SBSTREAM(bs,str) bstream bs((char*)str.data(), (int)str.size(), (int)str.size())
#define CHECK_SIGN_RETURNE(bs,sig,ret)  \
	signature_t sig; \
	hash_t ch_hash; \
	if (0 != bs >> sig) return ret; \
	cl::sha3_256(ch_hash.data(), 32, (uint8_t*)bs.read_ptr(), bs.length()); \
	if (!cryp::sig::verify(sig.id, sig.sign, ch_hash)) \
		return ret;

int tracker::get_rand_node(nodeaddr_t& n)
{
	if (m_referees.empty()) return -1;
	srand((uint32_t)time(NULL));
	int j = rand() % m_referees.size();
	int i;
	Guard l(m_mt);
	map<userid_t, tp_state_t>::iterator it;
	for (i = 0, it = m_referees.begin(); it != m_referees.end() && i < j; ++it, ++i);
	n = it->second.na;
	return 0;
}
int tracker::on_update_directors(const std::string& s)
{
	Guard l(m_mt);
	SBSTREAM(b, s);
	CHECK_SIGN_RETURNE(b, sig, -1);
	//只是白名单就行了
	if (!legalid(sig.id)) return -2;
	tran_names_t ns;
	if (0 == b >> ns)
	{
		if (m_direcs.height == 0 || ns.height > m_direcs.height)
			return update_namelist(ns, m_direcs, m_directors,DBK_DIRECTORS);
	}

	return -1;
}
int tracker::on_update_referees(const std::string& s)
{
	Guard l(m_mt);
	SBSTREAM(b, s);
	CHECK_SIGN_RETURNE(b, sig, -1);
	//只是白名单就行了
	if (!legalid(sig.id)) return -2;
	tran_names_t ns;
	if (0 == b >> ns)
	{
		if (m_refers.height==0 || ns.height > m_refers.height)
			return update_namelist(ns, m_refers, m_referees, DBK_REFEREES);
	}
	return -1;
}
int tracker::on_report_refstate(const std::string& s)
{
	Guard l(m_mt); 
	bstream b((char*)s.data(), (int)s.size(), (int)s.size());
	signature_t sig; 
	hash_t ch_hash; 
	if (0 != b >> sig) return -1; 
	cl::sha3_256(ch_hash.data(), 32, (uint8_t*)b.read_ptr(), b.length()); 
	if (!cryp::sig::verify(sig.id, sig.sign, ch_hash))
		return -1;
	map<userid_t, tp_state_t>::iterator it;
	string str;
	tp_state_t ti;
	bool bfind = false;
	if (0 == b >> ti)
	{
		ti.timestamp = time(NULL);
		it = m_directors.find(ti.na.id);
		if (it != m_directors.end())
		{
			it->second = ti;
			bfind = true;
		}
		it = m_referees.find(ti.na.id);
		if (it != m_referees.end())
		{
			it->second = ti;
			bfind = true;
		}
		if (bfind)
		{
			m_db->Put(m_wop, ti.na.id, cl::struct_to_string(ti, str, 4096));
		}
		return 0;
	}
	return -1;
}

int tracker::search_referees(map<userid_t, nodeaddr_t>& mp)
{
	Guard l(m_mt);
	map<userid_t, tp_state_t>::iterator it;
	mp.clear();
	for (it = m_referees.begin(); it != m_referees.end(); ++it)
	{
		mp[it->first] = it->second.na;
	}
	return 0;
}
int tracker::search_addr(const userid_t& id, nodeaddr_t& na)
{
	Guard l(m_mt);
	const tp_state_t* ti = find_tnodei(id);
	if (ti)
	{
		na = ti->na;
		return 0;
	}
	return -1;
}
int tracker::get_state(track_state_t& s)
{
	Guard l(m_mt);
	s.begintime = m_begintime;
	s.gc = m_gc;
	s.dirs = m_directors;
	s.refs = m_referees;
	return 0;
}
