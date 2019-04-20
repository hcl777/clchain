#include "secdb.h"
#include "util.h"


secdb::secdb()
	:m_db(NULL)
{
}


secdb::~secdb()
{
}

int secdb::init(const std::string& path)
{
	if (m_db) return 1;
	m_path = path;
	if (0 != sqlite3_open(m_path.c_str(), &m_db))
		return -1;
	return create_table(m_db);
}
void secdb::fini()
{
	close_db();
}
void secdb::close_db()
{
	if (m_db)
	{
		sqlite3_close(m_db);
		m_db = NULL;
	}
}
int secdb::reset_db()
{
	close_db();
	remove(m_path.c_str());
	if (0 != sqlite3_open(m_path.c_str(), &m_db))
		return -1;

	return create_table(m_db);
}
int secdb::create_table(sqlite3* db)
{
	int ret;
	string sql;
	char *emsg;
	//初始化创建表
	/* traflow
	每笔交易会产生多条记录,traidx从1开始算，0用于发行
	id,type,uid,desid,height,traidx,asset,time,balance
	type:
		1=块产出发行+手续费 （LC收入）
		2=LC收入（转账输入）
		3=LC支出（含手续费，转token时单独只有手续费）
		4=token收入
		5=token支出
	integer : 可存8字节数据
	*/
	sql = "create table if not exists tranflow ("
		"'id' integer primary key autoincrement,"
		"'type' integer not null, "
		"'uid' nvarchar(132) not null, "
		"'desid' nvarchar(132) not null,"
		"'height' integer not null, "
		"'traidx' integer not null, "
		"'asset' integer not null, "
		"'balance' integer not null,"
		"'time' datetime)"
		;
	ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, &emsg);
	if (SQLITE_OK != ret)
	{
		printf("#*** sqlite error: %s \n", emsg);
	}

	/*sendlog
	每笔交易一个记录
	id,uid,tokhash,height,traidx,fee,outs,ext,time
	outs： id:asset;id:asset; 最大512，
	*/
	sql = "create table if not exists tranlog ("
		"'id' integer primary key autoincrement,"
		"'uid' nvarchar(68) not null, "
		"'tokhash' nvarchar(68) not null,"
		"'height' integer not null, "
		"'traidx' integer not null, "
		"'fee' integer not null, "
		"'outs' nvarchar(512) not null,"
		"'ext' nvarchar(128) not null,"
		"'time' datetime)"
		; 
	ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, &emsg);
	if (SQLITE_OK != ret)
	{
		printf("#*** sqlite error: %s \n", emsg);
	}
	return 0;
}

int secdb::save_flowi(const string& id, const flow_i_t& f)
{
	if (!m_db) return -1;
	int ret;
	char *emsg, *sql;
	sql = sqlite3_mprintf("insert into tranflow(type,uid,desid,height,traidx,asset,balance,time)"
		"values(%d,'%q','%q',%llu,%d,%lld,%llu,'%q')",
		f.type, b2h(id).c_str(),f.desid.length()<16?f.desid.c_str():b2h(f.desid).c_str(), f.height,f.traidx, f.asset, f.balance, cl::time_to_datetime_string(f.t).c_str());
	ret = sqlite3_exec(m_db, sql, NULL, NULL, &emsg);
	if (SQLITE_OK != ret)
	{
		printf("#*** secdb::save_flowi() - %s\n", emsg);
	}
	sqlite3_free(sql);
	sqlite3_free(emsg);
	return ret;
}
int secdb::save_tranlog(const transaction_cert_t& tc,uint64_t h,int traidx)
{
	char out[1024] = { 0, };
	if (TRT_TRANSFER == tc.type)
	{
		uint64_t	n = 0;
		for (int i = 0; i < (int)tc.outs.size(); ++i)
			n += tc.outs[i].asset;
		if (tc.outs.size() != 1)
			snprintf(out, 1024, "multi_%d:%llu", (int)tc.outs.size(),(ull)n);
		else
			snprintf(out, 1024, "%s:%llu", b2h(tc.outs[0].id).c_str(), (ull)n);
	}
	else if (TRT_TOKEN == tc.type)
	{
		snprintf(out, 1024, "issued_token:%s", tc.tok.hash.to_hexString().c_str());
	}
	if(strlen(out)>0)
	{
		if (!m_db) return -1;
		int ret;
		char *emsg, *sql;
		sql = sqlite3_mprintf("insert into tranlog(uid,tokhash,height,traidx,fee,outs,ext,time)"
			"values('%q','%q',%llu,%d,%llu,'%q','%q','%q')",
			b2h(tc.id).c_str(), b2h(tc.tid).c_str(), h, traidx, tc.fee, out,tc.ext.c_str(), cl::time_to_datetime_string(tc.timestamp).c_str());
		ret = sqlite3_exec(m_db, sql, NULL, NULL, &emsg);
		if (SQLITE_OK != ret)
		{
			printf("#*** secdb::save_tranlog() - %s\n", emsg);
		}
		sqlite3_free(sql);
		sqlite3_free(emsg);
	}
	return 0;
}
int secdb::save_block_flow(block_t& b)
{
	Lock l(m_mt);
	for (map<string, flows_t>::iterator mit = b.mpf.begin(); mit != b.mpf.end(); ++mit)
	{
		for (list< flow_i_t>::iterator it = mit->second.ls.begin(); it != mit->second.ls.end(); ++it)
		{
			save_flowi(mit->first, *it);
		}
	}
	for (int i = 0; i < (int)b.trans.size(); ++i)
	{
		save_tranlog(b.trans[i].data, b.height, i);
	}
	return 0;
}
int secdb::get_flow(string& sret,const string& uid, const string& beg_date, const string& end_date)
{
	if (uid.empty() || beg_date.empty() || end_date.empty())
		return -1;
	Lock l(m_mt);
	int ret = 0;
	int nrow, ncol;
	char *emsg, *sql;
	char** b = NULL;
	char buf[2048],*p;
	sql = sqlite3_mprintf("select uid,desid,height,traidx,asset,balance,time from tranflow where uid='%q' and time>'%q' and time<'%q' limit 50",
		uid.c_str(), beg_date.c_str(), end_date.c_str());
	ret = sqlite3_get_table(m_db, sql, &b, &nrow, &ncol,&emsg);
	if (SQLITE_OK != ret)
	{
		printf("#*** secdb::get_flow() - %s\n", emsg);
		return -1;
	}

	if (nrow > 1)
	{
		int j = 0;
		sret = "[";
		for (int i = 1; i < nrow; i++)
		{
			j = i * ncol;
			p = buf;
			if (i > 0)
			{
				*p = ',';
				p++;
			}
			snprintf(p, 2040, R"({"uid":"%s","desid":"%s","idx":"%s-%s","asset":%s,"balance":%s,"time":"%s"})",
				b[j],b[j+1],b[j+2],b[j+3],b[j+4],b[j+5],b[j+6]);
			sret += buf;
		}
		sret += "]";
	}

	sqlite3_free_table(b);
	sqlite3_free(sql);
	sqlite3_free(emsg);
	return 0;
}
