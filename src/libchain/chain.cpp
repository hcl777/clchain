#include "chain.h"
#include "util.h"

#include "genesis.h"
#include "signature.h"
#include "leveldb/write_batch.h"
#include "secdb.h"


chain::chain()
	: speaker<chainListener>(2)
	, m_db(NULL)
{
	m_set.cid = 0;
	m_set.in_ex = 1;
	m_wop.sync = true;
	m_lbi.timestamp = 0;
	m_lbi.bt.height = INVALID_HEIGHT;
}


chain::~chain()
{
}


int chain::init(const std::string& dbpath, uint32_t cid)
{
	if (m_db)
		return 1;
	leveldb::Options options;
	leveldb::Status s;
	//options.error_if_exists = true;
	options.create_if_missing = true; //��Ϊ�����ڵ㲻��Ҫgenesis
	s = leveldb::DB::Open(options, dbpath, &m_db);
	if (!s.ok())
	{
		return -1;
	}

	m_set.cid = cid;
	string val;
	leveldb::ReadOptions rp;
#define DB2V(k,v) if (m_db->Get(rp, k, &val).ok()) string_to_struct(val, v)
	DB2V(DBK_CHAINSETTING, m_set);
	DB2V(DBK_LBI, m_lbi);
	DB2V(DBK_REFEREES, m_referees);
	DB2V(DBK_DIRECTORS, m_directors);
	DB2V(DBK_TOKENKEYS, m_tokenkeys);
	//m_db->Get(rp, DBK_TOKENKEYS, &m_stokenkeys);
	update_refereemp();

	m_cache.init();
	DEBUGMSG("CID:%d\n", m_set.cid);
	DEBUGMSG("UID:%s\n", b2h(comSngl::instance()->id()).c_str());
	return 0;
}
int chain::genesis(const std::string& jpath)
{
	block_t b;
	//bool bload = false;
	string str;
	if (0 == genesis_load_json(jpath, b))
	{
		if (0 == put_genesis_block(b,false))
		{
			DEBUGMSG("genesis() ok!\n");
			return 0;
		}
		else
		{
			DEBUGMSG("***put_genesis_block() wrong.\n");
		}
	}
	else
	{
		DEBUGMSG("***load genesis.json wrong.\n");
	}

	return -1;
}
void chain::fini()
{
	if (m_db)
	{
		delete m_db;
		m_db = NULL;
	}
	m_cache.fini();
}
void chain::update_refereemp()
{
	Lock l(m_mt);
	m_refereemp.clear();
	for (size_t i = 0; i < m_referees.names.size(); ++i)
		m_refereemp[m_referees.names[i]] = 0;
	comSngl::instance()->set_is_referee(m_refereemp.end() != m_refereemp.find(comSngl::instance()->id()));
	//DEBUGMSG("is_referee:%s\n", comSngl::instance()->is_referee()?"true":"false");
}
bool chain::is_director(const userid_t& id)
{
	Lock l(m_mt);
	for (size_t i = 0; i < m_directors.names.size(); ++i)
	{
		if (id == m_directors.names[i])
			return true;
	}
	return false;
}
//string chain::find_key_from_map(const userid_t& id, map<userid_t, string>& mp)
//{
//	string s;
//	map<userid_t, string>::iterator it = mp.find(id);
//	if (it != mp.end())
//		s = it->second;
//	return s;
//}
//string chain::find_authorizer_key(const userid_t& id)
//{
//	for (size_t i = 0; i < m_directors.size(); ++i)
//	{
//		if (m_directors[i].id == id)
//			return m_directors[i].pubkey;
//	}
//	return "";
//}
//string chain::find_key(const userid_t& id)
//{
//	string s;
//	map<userid_t, string>::iterator it = m_referees.find(id);
//	if (it != m_referees.end())
//		s = it->second;
//	else
//	{
//		//�ӵ�ַ���
//		m_db->Get(leveldb::ReadOptions(), id, &s);
//	}
//	return s;
//}


int chain::find_utxo_assets(const string& id, assets_t& as)
{
	Lock l(m_mt);
	string k,v;
	if (m_db->Get(leveldb::ReadOptions(), id, &v).ok())
	{
		return string_to_struct(v, as);
	}
	return -1;
}
int chain::make_uas(vector<user_assets_t>& uas, map<string, int64_t>& asmp,uint64_t height)
{
	map<string, int64_t>::iterator asit;
	assets_t as;
	size_t i;
	uas.resize(asmp.size());
	for (i = 0, asit = asmp.begin(); asit != asmp.end(); ++asit, ++i)
	{
		//�ҳ�ID��Ӧ������ָ��
		if (0 != chainSngl::instance()->find_utxo_assets(asit->first, as))
		{
			//û�����ˣ����ʼΪ0��ָ�򱾿�
			as.assets = 0;
			as.height = height;
		}
		uas[i].id = asit->first;
		//�жϲ���֧��
		if (asit->second < 0 && (long long)as.assets < llabs(asit->second))
			return BERR_ASSETS;
		uas[i].as.assets = as.assets + asit->second;
		uas[i].as.height = as.height;
	}
	return 0;
}
int chain::make_uas2(vector<user_assets_t>& uas, map<string, flows_t>& mp)
{
	map<string, flows_t>::iterator it;
	size_t i;
	uas.resize(mp.size());
	for (i = 0, it = mp.begin(); it != mp.end(); ++it, ++i)
	{
		uas[i].id = it->first;
		uas[i].as.assets = it->second.balance;
		uas[i].as.height = it->second.last_height;
	}
	return 0;
}
/*
���Ҳ���in_height~ in_height+in_ex ��Χ���Ƿ���ڶ�Ӧ�Ľ���
���أ�
	-1������ʧ�ܣ������ڣ�
	0��δ֪�����滹���ܻ����������¼��
	1�����׳ɹ������ڣ�
*/
int chain::find_tran(const hash_t& tranhash, uint64_t in_height)
{
	Lock l(m_mt);
	uint64_t h;
	map<uint64_t, map<hash_t, char>>::iterator it;
	vector<hash_t> v;
	for (int i = 0; i <= m_set.in_ex; ++i)
	{
		h = in_height + i;
		if (h > m_lbi.bt.height)
			return 0; //δ֪
		it = m_tranhashs.find(h);
		if (it != m_tranhashs.end())
		{
			if (it->second.find(tranhash) != it->second.end())
				return 1;
		}
		else
		{
			if (0 == get_tranhash_from_db(h, v))
			{
				if (std::find(v.begin(), v.end(), tranhash) != v.end())
					return 1;
			}
			else
				return 0; //���ݻ�������Զδ֪
		}
	}
	return -1;
}

/*
��飺 cid,ǩ����in_height�Ƿ���ڡ�

*/
int chain::check_tran_ok(transaction_t& t)
{
	//�ڴ˲�ʹ������Ҫ�ر�ע�ⰲȫ

	//���Ϸ������������ڴ�Ԥ����չ����ģ��
	if (t.data.cid != m_set.cid)
		return BERR_ID;
	if (!cryp::sig::verify(t.data.id, t.sign, t.tranhash))
		return BERR_TRAN_HASH;

	//֮ǰ�Ƿ����
	if (0 != find_tran(t.tranhash, t.data.in_height))
		return BERR_TRAN;
	
	//������
	//���ʽ��׼��֧���Ƿ�û������
	if (TRT_TRANSFER == t.data.type)
	{
		//���fee
		if (t.data.fee < m_set.min_tranfee)
			return BERR_FEE;
	}
	else if (TRT_DIRECTOR == t.data.type)
	{
		//��齻��ID�Ƿ�Ϊ��1ID
		if (m_directors.names[0] != t.data.id)
			return BERR_DIRECTOR;
		if(t.data.ns.height!= m_directors.height)
			return BERR_DIRECTOR;
	}
	else if (TRT_REFEREE == t.data.type)
	{
		if(!is_director(t.data.id))
			return BERR_REFEREE;
		if (t.data.ns.height != m_directors.height)
			return BERR_DIRECTOR;
	}
	else if (TRT_TOKEN == t.data.type)
	{
		//���tok_hash������
		//todo: δ��鱾�����Ƿ����ͬ����hash
		if (exist_token(DBK_TOKENPRE + t.data.tok.hash.to_string()))
			return BERR_TOKEN;

		//���fee
		if (t.data.fee < m_set.min_consfee)
			return BERR_FEE;

		//���token
		tran_token_t& tok = t.data.tok;
		if (tok.cid != m_set.cid)
			return BERR_ID;
		if (t.data.id != tok.id)
			return BERR_TOKEN;
		//���ǩ��
		if (!cryp::sig::verify(tok.id, tok.pub_sign,tok.hash))
			return BERR_TRAN_HASH;
		//�����Ȩ�Ϸ���
		if (!cryp::sig::verify(tok.director_sign.id, tok.director_sign.sign, tok.hash))
			return BERR_TRAN_HASH;
		if (!is_director(tok.director_sign.id))
			return BERR_TOKEN;
	}
	return BERR_OK;
}

/*
2��������������Ϊdirectors ��referee����һ������ΪTRT_DIRECTOR��referee;

*/
int chain::put_genesis_block(block_t& b, bool check_cid/* = true*/)
{
	Lock l(m_mt);
	//�鷢����ǩ��
	hash_block(b); //����������Ҫ��֤��hash
	if (!cryp::sig::verify(b.pub_out.id, b.pub_sign, b.blockhash))
		return BERR_PUB_SIGN;
	uint32_t cid = cl_crc32_write(CL_CRC32_FIRST, b.blockhash.data(), (int)b.blockhash.size())&0x7fffffff;
	if(check_cid)
	{
		if (cid != m_set.cid)
			return BERR_ID;
	}
	else
	{
		m_set.cid = cid;
	}
	
	//�׿�ֻ��pub_out��1���ַ���ף�û��ͶƱ
	if (b.height != m_lbi.bt.height + 1 || b.pre_hash != m_lbi.bt.blockhash)
		return BERR_PRE;
	if (b.vote_signs.size()!=1 || b.trans.size() != 1 || b.trans[0].data.type != TRT_DIRECTOR)
		return BERR_TRAN;

	//pub_sign Ϊ��������
	genesis_chain_setting(b.vote_signs.front().sign, m_set);

	string str1, str2;
	cpo_last_t lbi;
	assets_t as;
	tran_names_t directors, referees;

	lbi.bt.height = b.height;
	lbi.bt.blockhash = b.blockhash;
	lbi.timestamp = b.timestamp;

	directors.height = b.height;
	directors.names = b.trans[0].data.ns.names;
	referees = directors;

	//reset sqlite
	secdbSngl::instance()->reset_db();

	//����飬ʹ��ԭ�Ӳ���
	leveldb::Status s;
	leveldb::WriteBatch batch;

	//setting
	batch.Put(DBK_CHAINSETTING, struct_to_string(m_set, str1, 1024));
	
	//utxo
	as.height = b.height;
	as.assets = b.pub_out.asset;
	batch.Put(b.pub_out.id, struct_to_string(as, str2, 128));
	secdbSngl::instance()->save_flowi(b.pub_out.id, flow_i_t(FT_MINER_IN, "genesis", b.height, 0, as.assets, as.assets, b.timestamp));
	//as.assets = 0;
	//for(size_t i=1;i<directors.names.size();++i)
	//	batch.Put(directors.names[i], struct_to_string(as, str2, 128));
	
	//��
	batch.Put(struct_to_string(b.height, str1, 8), struct_to_string(b, str2, MAX_BLOCK_SIZE));
	//������Ϣ
	batch.Put(DBK_LBI, struct_to_string(lbi, str2, 128));
	//referees
	batch.Put(DBK_REFEREES, struct_to_string(referees, str2, 1024000));
	//m_directors
	batch.Put(DBK_DIRECTORS, struct_to_string(directors, str2, 102400));
	//

	s = m_db->Write(m_wop, &batch);
	if (s.ok())
	{
		//update pointer
		m_referees = referees;
		m_directors = directors;
		m_lbi = lbi;
		update_refereemp();
		fire(chainListener::UpdateReferee());
		fire(chainListener::UpdateDirectors());
		fire(chainListener::UpdateLBI());

		try_put_block_from_cache();
		return BERR_OK;
	}
	//end
	return BERR_DB_FAILD;
}
flows_t& chain::find_flows(block_t& b, const string& id)
{
	flows_t& fs = b.mpf[id];
	if (fs.ls.empty())
	{
		assets_t as;
		if (0 == chainSngl::instance()->find_utxo_assets(id, as))
		{
			fs.balance = as.assets;
			fs.last_height = as.height;
		}
		else
		{
			fs.balance = 0;
			fs.last_height = b.height;
		}
	}
	return fs;
}
/*
//����ID
//���ָ��
//��blockhash
//�鷢����ǩ��
//��ͶƱ��ǩ��
//�齻����Ϣ
*/
int chain::check_block_ok(block_t& b,bool bcheck_votesign/*=true*/)
{
	Lock l(m_mt);
	int ret;
	if (b.cid != m_set.cid)
		return BERR_ID;
	//��ָ֤��
	if (b.height != m_lbi.bt.height + 1 || b.pre_hash != m_lbi.bt.blockhash || b.timestamp <= m_lbi.timestamp)
		return BERR_PRE;
	if (b.tranhashs.size()!=b.trans.size()) return BERR_TRAN;

	hash_block_all(b);

	//��֤����hash�����ظ�
	{
		map<hash_t, int> mp;
		map<hash_t, int>::iterator it;
		for (size_t i = 0; i < b.trans.size(); ++i)
		{
			if (b.tranhashs[i] != b.trans[i].tranhash)
				return BERR_TRAN;
			if (mp.find(b.trans[i].tranhash) != mp.end())
				return BERR_TRAN;
			mp[b.trans[i].tranhash] = 1;
		}
	}

	string pubkey;
	//�鷢����ǩ��,��һ�ʵ�ID
	if (!cryp::sig::verify(b.pub_out.id, b.pub_sign, b.blockhash))
		return BERR_PUB_SIGN;

	if (bcheck_votesign)
	{
		//��ͶƱ��ǩ��������ǩ���Ĳ�ȫ�ǻ�Ա�����ֱ��ýڵ㣬����������
		if (b.vote_signs.size() < min_vote_num()) return BERR_VOTE_SIGN;
		for (list<signature_t>::iterator it = b.vote_signs.begin(); it != b.vote_signs.end(); ++it)
		{
			signature_t& sig = *it;
			if(m_refereemp.find(sig.id)== m_refereemp.end())
				return BERR_VOTE_SIGN;
			if (!cryp::sig::verify(sig.id, sig.sign, b.blockhash))
				return BERR_VOTE_SIGN;
		}
	}

	//flow
	b.mpf.clear();

	//�齻��
	size_t i,j;
	uint64_t fee = 0;
	for (i = 0; i < b.trans.size(); ++i)
	{
		transaction_cert_t& tc = b.trans[i].data;
		//�޶�����ָ����
		//if (TRT_TRANSFER == b.trans[i].data.type && 0 != b.trans[i].data.in_height)
		{
			//������Ƿ���ȷ
			if (b.height<tc.in_height ||b.height>tc.in_height+ m_set.in_ex)
				return BERR_TRAN;
		}
		ret = check_tran_ok(b.trans[i]);
		if (BERR_OK != ret) return ret;
		fee += tc.fee;

		//flow
		string fromid,toid;
		fromid = tc.to_asid();
		flows_t& ofs = find_flows(b,fromid);
		if (TRT_TRANSFER == tc.type)
		{
			//�ȼ�¼������ˮ
			for (j = 0; j < tc.outs.size(); ++j)
			{
				toid = tc.to_outid(tc.outs[i].id);
				flows_t& fs = find_flows(b, toid);
				if (ofs.balance < tc.outs[j].asset) 
					return BERR_ASSETS;
				fs.balance += tc.outs[j].asset;
				fs.ls.push_back(flow_i_t(tc.tid.empty() ? FT_LC_IN : FT_TOK_IN, fromid, b.height, (int)i, tc.outs[j].asset, fs.balance, tc.timestamp));
				ofs.balance -= tc.outs[j].asset;
				ofs.ls.push_back(flow_i_t(tc.tid.empty() ? FT_LC_OUT : FT_TOK_OUT, toid, b.height, (int)i, -(int64_t)tc.outs[j].asset, ofs.balance, tc.timestamp));
			}
		}
		else if (TRT_TOKEN == tc.type)
		{
			//����Ϊȫ��
			flows_t& fs = b.mpf[tc.id+ tc.tok.hash.to_string()];
			fs.balance = tc.tok.assets; 
			fs.last_height = b.height;
			fs.ls.push_back(flow_i_t(FT_TOK_IN,"issued token",b.height,(int)i,tc.tok.assets,fs.balance,tc.timestamp));
		}
		//��¼֧��fee
		if (tc.fee>0)
		{
			flows_t& ffs = find_flows(b, tc.id);
			if(ffs.balance<tc.fee)
				return BERR_ASSETS;

			ffs.balance -= fee;
			ffs.ls.push_back(flow_i_t(FT_FEE_OUT, "fee out", b.height, (int)i, -(int64_t)tc.fee, ffs.balance, tc.timestamp));
		}

	}

	//���ױʲ�������
	if (b.pub_out.asset != BLOCK_OUTPUT + fee)
		return BERR_TRAN_PUBOUT;

	//flow
	flows_t& ffs = find_flows(b, b.pub_out.id);
	ffs.balance += b.pub_out.asset;
	ffs.ls.push_back(flow_i_t(FT_MINER_IN, "miner", b.height, 0, b.pub_out.asset, ffs.balance, b.timestamp));

	//�����˱�
	vector<user_assets_t> uas;
	//map<string, int64_t> asmp;
	//map<string, int64_t>::iterator asit;
	//ret = make_block_assets(asmp, b.pub_out, b.trans); 
	//if (0 != ret) return ret;
	//ret = make_uas(uas, asmp, b.height); 
	ret = make_uas2(uas, b.mpf);
	if (0 != ret) return ret;
	
	if(uas.size()!=b.uas.size())
		return BERR_ASSETS;
	for (i = 0; i < uas.size(); ++i)
	{
		if (uas[i] != b.uas[i])
			return BERR_ASSETS;
	}
	
	//check end
	return BERR_OK;
}

int chain::put_block(char *buf, int size, bool btry_cache/* = true*/)
{
	Lock l(m_mt);
	block_t b;
	uint64_t height = 0;
	bstream s(buf + 5, size - 5, size - 5);
	if (0 != s >> height)
		return -1;
	if (height < m_lbi.bt.height + 1)
		return BERR_PRE;
	else if (height > m_lbi.bt.height + 1)
	{
		if (btry_cache)
			return m_cache.put(height, buf, size);
		assert(false);
		return -1;
	}
	//==
	s.attach(buf, size, size);
	if (0 != s >> b)
		return -1;
	return put_block(b);
}
int chain::put_block(block_t& b)
{
	Lock l(m_mt);
	int ret;
	size_t i;
	if (1 == b.height)
		return put_genesis_block(b);
	if (b.height != m_lbi.bt.height + 1)
	{
		assert(false);
		return -1;
	}

	ret = check_block_ok(b);
	if (BERR_OK != ret)
		return ret;

	//*********************************
	//�����
	//����pointer:: lbi,addr,referees,directors,assets,token,
	leveldb::Status dbs;
	leveldb::WriteBatch batch;

	cpo_last_t lbi;
	tran_names_t directors, referees;
	assets_t as;
	string str1, str2;
	string	tokenkey;
	//string tokenkeys;
	list<string> tokenkeys;

	lbi.bt.height = b.height;
	lbi.bt.blockhash = b.blockhash;
	lbi.timestamp = b.timestamp;

	directors.height = referees.height = b.height;

	//��
	batch.Put(struct_to_string(b.height, str1, 8), struct_to_string(b, str2, MAX_BLOCK_SIZE));
	//������Ϣ
	batch.Put(DBK_LBI, struct_to_string(lbi, str2, 128));
	//utxo
	for (i = 0; i < b.uas.size(); i++)
	{
		as.assets = b.uas[i].as.assets;
		as.height = b.height;
		batch.Put(b.uas[i].id, struct_to_string(as, str2, 128));
	}
	for (i = 0; i < b.trans.size(); i++)
	{
		transaction_cert_t& tc = b.trans[i].data;
		
		if (tc.type == TRT_TOKEN)
		{
			tokenkey = DBK_TOKENPRE+tc.tok.hash.to_string();
			batch.Put(tokenkey, struct_to_string(tc.tok, str2, 1024000));
			if (tokenkeys.empty()) tokenkeys = m_tokenkeys;
			tokenkeys.push_back(tokenkey);
			//tokenkey_add(tokenkeys, tokenkey);
		}
		else if (tc.type == TRT_DIRECTOR)
		{
			directors.names = tc.ns.names;
			batch.Put(DBK_DIRECTORS, struct_to_string(directors, str2, 1024000));
		}
		else if (tc.type == TRT_REFEREE)
		{
			referees.names = tc.ns.names;
			batch.Put(DBK_REFEREES, struct_to_string(referees, str2, 1024000));
		}
	}
	//tokenkeys
	if (!tokenkeys.empty())
	{
		batch.Put(DBK_TOKENKEYS, struct_to_string(tokenkeys, str2, (int)(37*tokenkeys.size())+20));
		//batch.Put(DBK_TOKENKEYS, tokenkeys);
	}

	//flow
	secdbSngl::instance()->save_block_flow(b);

	dbs = m_db->Write(m_wop, &batch);
	//���뱣��д��DB��Ÿ����ڴ�
	if (dbs.ok())
	{
		//update pointer
		//����last
		m_lbi = lbi;
		//����referees
		if (!referees.names.empty())
		{
			m_referees = referees;
			update_refereemp();
			fire(chainListener::UpdateReferee());
		}
		//����directors
		if (!directors.names.empty())
		{
			m_directors = directors;
			fire(chainListener::UpdateDirectors());
		}
		//tokenkeys
		if (!tokenkeys.empty())
			m_tokenkeys.swap(tokenkeys);

		fire(chainListener::UpdateLBI()); //�������UpdateReferee ����
		cache_tranhash(b.height, b.tranhashs);

		try_put_block_from_cache();
		return BERR_OK;
	}
	
	return BERR_DB_FAILD;
}
void chain::try_put_block_from_cache()
{
	//���Ի�ȡcache ����
	string *next = m_cache.get(m_lbi.bt.height + 1);
	if (NULL != next)
	{
		put_block((char*)next->data(), (int)next->size(), false);
		//ִ�м����������Ƿ�ɹ���ɾ��cache,����cache����Ŀ�һֱ���²�����
		delete next;
	}
	m_cache.del_old(m_lbi.bt.height);
}
int chain::rollback_to(uint64_t height)
{
	Lock l(m_mt);
	assert(0);
	return -1;
}
bool chain::get_block(uint64_t i, string& s)
{
	Lock l(m_mt);
	string str1;
	leveldb::Slice sl;
	if (i > m_lbi.bt.height) return false;
	return m_db->Get(leveldb::ReadOptions(), struct_to_string(i, str1, 8), &s).ok();
}

//void chain::tokenkey_add(string& keys, const string& k)
//{
//	string s;
//	s.resize(k.size() + 4);
//	uint32_t size = bstream::htol32((uint32_t)k.size());
//	memcpy((void*)s.data(), (void*)&size, 4);
//	memcpy((void*)(s.data()+4), k.data(), k.size());
//}
int chain::get_sync_task(uint64_t maxh, list<uint64_t>& ls)
{
	Lock l(m_mt);
	/*���ȡ10���ڵ�����*/
	if (maxh <= m_lbi.bt.height)
		return 0;
	return m_cache.get_sync_task(m_lbi.bt.height+1, min(maxh, m_lbi.bt.height+10), ls);
}
bool chain::exist_token(const string& tokid)
{
	for (list<string>::iterator it = m_tokenkeys.begin(); it != m_tokenkeys.end(); ++it)
	{
		if (tokid == (*it))
			return true;
	}
	return false;
}
int chain::cache_tranhash(uint64_t h, vector<hash_t>& v)
{
	while (m_tranhashs.size() > 9)
		m_tranhashs.erase(m_tranhashs.begin());
	map<hash_t, char>& mp = m_tranhashs[h];
	mp.clear();
	for (vector<hash_t>::iterator it = v.begin(); it != v.end(); ++it)
		mp[*it] = 0;
	return 0;
}
int chain::get_tranhash_from_db(uint64_t h, vector<hash_t>& v)
{
	string val,k;
	leveldb::ReadOptions rp;
	block_t i;
	if (m_db->Get(rp, struct_to_string(h, k, 8), &val).ok())
	{
		v.clear();
		bstream s((char*)val.data(), (int)val.size(), (int)val.size());
		s >> i.cid;
		s >> i.ver;
		s >> i.height;
		s >> i.pre_hash;
		s >> i.timestamp;
		s >> v;
		return s.ok();
	}
	return -1;
}

