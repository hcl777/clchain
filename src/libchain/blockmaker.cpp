#include "blockmaker.h"
#include "chain.h"
#include "signature.h"

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

blockmaker::blockmaker()
	:m_make_block(0)
{
}


blockmaker::~blockmaker()
{
}
int blockmaker::run(int unmkblock)
{
	DEBUGMSG("blockmaker::unmkblock = %d\n", unmkblock);
	m_unmkblock = unmkblock;
	if (m_unmkblock)
		return 0;
	activate(2);
	return 0;
}
void blockmaker::end()
{
	m_brun = false;
	wait();

	list<transaction_t*>::iterator lsit;
	map<string, transaction_t*>::iterator mpit;
	for (lsit = m_ls.begin(); lsit != m_ls.end(); ++lsit)
		delete *lsit;
	m_ls.clear();
	for (mpit = m_mp.begin(); mpit != m_mp.end(); ++mpit)
		delete mpit->second;
	m_mp.clear();
}
void blockmaker::work(int e)
{
	if (0 == e)
		thread_tran();
	else
		thread_block();
}

void blockmaker::begin_make_block()
{
	m_make_block = 1;
}
int blockmaker::try_get_block(block_t& b)
{
	if (2 == m_make_block && m_block.height==chainSngl::instance()->get_last_height()+1)
	{
		Guard l(m_mtls);
		b = m_block;
		m_make_block = 0;
		return 0;
	}
	return -1;
}
void blockmaker::end_make_block()
{
	m_make_block = 0;
}
int blockmaker::add_trans(const transaction_t& t)
{
	Guard l(m_mtls);
	if (m_unmkblock || m_ls.size()>10000||m_mp.size()>10000)
		return 0;
	transaction_t* tc = new transaction_t();
	*tc = t;
	m_ls.push_back(tc);
	return 0;
}

void blockmaker::thread_tran()
{
	//DEBUGMSG("blockmaker::thread_tran()\n");
	transaction_t* t;
	char buf[1024];
	string k;
	while (m_brun)
	{
		if (m_ls.empty())
		{
			msleep(50);
			continue;
		}
		{
			Guard l(m_mtls);
			if (m_ls.empty())
				continue;
			t = m_ls.front();
			m_ls.pop_front();
		}
		hash_transaction(*t);
		if (0 == chainSngl::instance()->check_tran_ok(*t))
		{
			Guard l(m_mtmp);
			sprintf(buf, "%llu%s", (unsigned long long)t->data.timestamp, (char*)t->tranhash.data());
			k = buf;
			if (m_mp.find(k) == m_mp.end())
				m_mp[k] = t;
			else
				delete t;
		}
		else
			delete t;
	}
}
void blockmaker::thread_block()
{
	msleep(1000);
	//test_make_block();
	//DEBUGMSG("blockmaker::thread_block()\n");
	map<string, transaction_t*> mp;
	while (m_brun)
	{
		msleep(50);
		if (1 != m_make_block)
			continue;

		{
			Guard l(m_mtmp);
			mp.swap(m_mp);
		}
		//test 暂允许空块方便测试
		if (mp.empty())
			continue;
		{
			
			ecuser_t u;
			u.pubk = comSngl::instance()->id();
			u.prik = comSngl::instance()->prikey();
			block_t b;
			if (0 == make_block(b,u, mp))
			{
				Guard l(m_mtls);
				if (1 == m_make_block)
				{
					m_block = b;
					m_make_block = 2;
				}
			}
		}
		for (map<string, transaction_t*>::iterator it = mp.begin(); it != mp.end(); ++it)
			delete it->second;
		mp.clear();
	}
}
/*
mp 查找记录本块涉及支出的账户余额。
*/
uint64_t& find_ua(map<string, uint64_t>& mp, const userid_t& id)
{
	assets_t as;
	
	map<string, uint64_t>::iterator asit = mp.find(id);
	if (asit != mp.end()) return asit->second;
	if (0 == chainSngl::instance()->find_utxo_assets(id, as))
	{
		uint64_t& u = mp[id];
		u = as.assets;
		return u;
	}
	//账户不存在的记0
	uint64_t& u = mp[id];
	u = 0;
	return u;
}
/*
暂时允许空块方便测试
*/
int blockmaker::make_block(block_t& b, ecuser_t& u, map<string, transaction_t*>& mp)
{
	transaction_t* t;
	size_t i;

	list<transaction_t*> ls, ls2;
	list<transaction_t*>::iterator lsit;
	map<string, transaction_t*>::iterator it;
	map<string, uint64_t> imp;
	map<string, int64_t> asmp;
	//map<string, int64_t>::iterator asit;
	
	cpo_last_t lbi;
	uint8_t in_ex = chainSngl::instance()->get_in_ex();

	//提前获取height等信息，后面生成块过程中即使chain有变化，height可以快速判定是否错误
	b.cid = chainSngl::instance()->cid();
	chainSngl::instance()->get_last_blockinfo(lbi);
	b.height = lbi.bt.height + 1;
	b.pre_hash = lbi.bt.blockhash;
	
	uint64_t timestamp = time(NULL);
	for (it = mp.begin(); it != mp.end(); ++it)
	{
		t = it->second;
		long long tmp = llabs(timestamp - t->data.timestamp);
		if (tmp>180)
		{
			DEBUGMSG("****** make block tran.timestamp timeout(%lld sec)!*****\n", tmp);
			continue;
		}
		if(b.height<t->data.in_height || b.height>t->data.in_height+in_ex)
		{
			//当前块不符合插入范围
			continue;
		}
		if (0 != chainSngl::instance()->find_tran(t->tranhash, t->data.in_height))
		{
			//1表示交易已经存在，-1表示in_height已经越界
			continue;
		}
		ls.push_back(t);
	}

	//检查支出OK，记录支出的用户余额
	for (lsit = ls.begin(); lsit != ls.end(); ++lsit)
	{
		t = *lsit;
		uint64_t& ua = find_ua(imp, t->data.to_asid());
		uint64_t& uafee = find_ua(imp, t->data.id); //可能与ua是同一个
		if (t->data.fee > uafee) continue;
		uafee -= t->data.fee;
		if (TRT_TRANSFER == t->data.type)
		{
			for (i = 0; i < t->data.outs.size(); ++i)
			{
				//在此不能累加支出，避免溢出漏洞。采用递减法
				if (t->data.outs[i].asset == 0 || t->data.outs[i].asset > ua)
					continue;
				ua -= t->data.outs[i].asset;
			}
		}
		ls2.push_back(t);
	}
	//test: 测试同步时先不允许生成空块
	if (ls2.empty())
		return -1;

	//此时认为所有ls2 OK
	
	b.trans.resize(ls2.size());
	b.tranhashs.resize(ls2.size());
	//计账收入
	b.pub_out.id = u.pubk;
	b.pub_out.asset = BLOCK_OUTPUT;
	for (i = 0, lsit = ls2.begin(); lsit != ls2.end(); ++lsit, ++i)
	{
		t = *lsit;
		b.trans[i] = *t;
		b.tranhashs[i] = t->tranhash;
		b.pub_out.asset += t->data.fee;
	}
	
	b.timestamp = std::time(NULL);
	
	asmp.clear();
	if (0 != make_block_assets(asmp, b.pub_out, b.trans))
		return -1;
	if (0 != chainSngl::instance()->make_uas(b.uas, asmp, b.height))
		return -1;
	//最后判断一下height是否发生了变化
	if (b.height != chainSngl::instance()->get_last_height() + 1)
		return -1;
	hash_block(b);
	cryp::sig::sign(u.prik, b.blockhash, b.pub_sign);
	return 0;
}
void blockmaker::test_make_block()
{
	string str;
	map<string, transaction_t*> mp;
	vector<tran_out_t>	outs;
	block_t b1, b2,b3;
	//int ret = 0;
	ecuser_t u1,u2,u3,u4;
	signature_t sig;
	uint32_t cid = chainSngl::instance()->cid();
	string tokid = h2b("a18f8d3fb2600ef789afa5c4eb842478f4bb20f65c8f9ca82db8716d87813aa9");
	u1.pubk = comSngl::instance()->id();
	u1.prik = comSngl::instance()->prikey();
	u2.prik = "5d1e77c6341aa874da9670d84b5b723a619a928f11dd473cf94a69c06103c51e";
	u2.pubk = "e53db328cd199bfff532f3027ccb34f2d7c9898cbbeb53caf5000494e928b17b";
	u2 = u2.to_bytes();
	u3.prik = "854d500663fe7ec5b83e8f8ddc032efc81744abb0bf3de6a25204d671f8c3e4d";
	u3.pubk = "61a7f432857b553a25de015e1cb045378db27c534d57efd6decca2b75056c99e";
	u3 = u3.to_bytes();
	if (!cryp::sig::check_key(u1.prik, u1.pubk))
		return;
	if (!cryp::sig::check_key(u2.prik, u2.pubk))
		return;
	//u4 = u1; u1 = u2; u2 = u4;
	outs.resize(1);

	transaction_t t1,t2,t3;
	//outs[0].id = u2.pubk; outs[0].asset = 50000;
	//trans::new_tran(str, cid, u3,tokid,10, 2, outs);
	//string_to_struct(str, t1);
	//hash_transaction(t1);
	//mp["1"] = &t1;

	//outs[0].id = u3.pubk; outs[0].asset = 1000;
	//trans::new_tran(str, cid, u3, tokid, 4, 2, outs);
	//string_to_struct(str, t2);
	//hash_transaction(t2);
	//mp["2"] = &t2;

	////发行tok
	//trans::new_token(str, cid, u3, u2, 2, 2000000000000, "dianjin", "wang ba dian jin!.", 600);
	//string_to_struct(str, t3);
	//hash_transaction(t3);
	//mp["3"] = &t3;

	//director改为3个，referee改为4个。
	tran_names_t ns,ns2;
	ns.height = chainSngl::instance()->get_directors_height();
	ns.names.push_back(h2b("e53db328cd199bfff532f3027ccb34f2d7c9898cbbeb53caf5000494e928b17b"));
	ns.names.push_back(h2b("e2b2123de48c8774fc3f59c4de49d26cc348626f0332d94ddeca0445eff099e7"));
	ns.names.push_back(h2b("61a7f432857b553a25de015e1cb045378db27c534d57efd6decca2b75056c99e"));
	trans::new_names(t1, cid, u1, 28, ns,TRT_DIRECTOR);

	ns2.height = chainSngl::instance()->get_referees_height();
	ns2.names.push_back(h2b("e53db328cd199bfff532f3027ccb34f2d7c9898cbbeb53caf5000494e928b17b"));
	ns2.names.push_back(h2b("e2b2123de48c8774fc3f59c4de49d26cc348626f0332d94ddeca0445eff099e7"));
	ns2.names.push_back(h2b("61a7f432857b553a25de015e1cb045378db27c534d57efd6decca2b75056c99e"));
	ns2.names.push_back(h2b("a5912122f4de17642384d39fae08e3e8be291ea5171c641fe248eee9bc9d4dbd"));
	trans::new_names(t2, cid, u2, 28, ns2, TRT_REFEREE);

	add_trans(t1);
	add_trans(t2);

	//if (0 == make_block(b1,u2, mp))
	//{
	//	b2 = b1;
	//	sig.id = u2.pubk;
	//	cryp::sig::sign(u2.prik, b2.blockhash, sig.sign);
	//	b2.vote_signs.push_back(sig);
	//	cl::struct_to_string(b2, str, 1024000);
	//	cl::string_to_struct(str, b3);
	//	ret = chainSngl::instance()->put_block(b3);
	//}
}
