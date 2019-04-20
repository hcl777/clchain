#include "proposer.h"

#include "sorter.h"
#include "clsha3.h"
#include "signature.h"
#include "cltype.h"
#include "blockmaker.h"

proposer::proposer() 
{
	m_timeout_ms = 3000;
	m_myindex = -1;
	m_last_have_given_tick = mtick();
	last_give_blocksign_tick = 0;
	last_give_blocksign_height = -1;
	reset_state();
}
void proposer::reset_state(int icrr/*=0*/,int state/*=IDLE*/)
{
	if (m_bmy)
	{
		DEBUGMSG("%.1f: pro_endr\n", mtick() / (double)1000);
		blockmakerSngl::instance()->end_make_block();
	}
	set_state(state);
	m_icrr = icrr;
	m_icrr_begin_tick = mtick();

	m_votes.clear();
	m_bsigns.clear();
	m_bmy = false;
}
void proposer::update_referees(map<userid_t, int>& r)
{
	//插入第一个空值
	m_refs.resize(r.size() + 1);
	map<userid_t, int>::iterator it;
	int i = 1;
	for (it = r.begin(); it != r.end(); ++it, ++i)
	{
		m_refs[i].id = it->first;
	}
}
void proposer::update_lbi()
{
	chainSngl::instance()->get_last_blockinfo(m_lbi);
	reset_state();
	sort_ids();
	//计算myindex
	m_myindex = -1;
	for (int i = 1; i < (int)m_refs.size(); ++i)
	{
		if (m_refs[i].id == comSngl::instance()->id())
		{
			m_myindex = i;
			break;
		}
	}
}
string operator^(const string& id, const hash_t& hash)
{
	string s = id;
	size_t size = min(id.size(), hash.size());
	for (size_t i = 0; i < size; ++i)
	{
		s.at(i) = s.at(i) ^ hash.at(i);
	}
	return s;
}
void proposer::sort_ids()
{
	//第0个不排序
	int ibegin = 1;
	int i, j, k = (int)m_refs.size() - 1;
	proinfo_t tmp;
	userid_t id1, id2;
	for (j = k; j>ibegin; j = k)
	{
		k = ibegin;
		for (i = ibegin; i<j; ++i)
		{
			id1 = m_refs[i+1].id ^ m_lbi.bt.blockhash;
			id2 = m_refs[i].id ^ m_lbi.bt.blockhash;
			if (id1<id2)
			{
				tmp = m_refs[i];
				m_refs[i] = m_refs[i + 1];
				m_refs[i + 1] = tmp;
				k = i;
			}
		}
	}
	//DEBUGMSG("proposer sort(%d):\n", (int)m_refs.size());
	//for (i = 0; i < (int)m_refs.size(); ++i)
	//{
	//	DEBUGMSG("		%2d:%s\n",i, b2h(m_refs[i].id).c_str());
	//}
}
int proposer::find_index(const userid_t& id)
{
	for (int i=1; i < (int)m_refs.size(); ++i)
	{
		if (m_refs[i].id == id)
			return i;
	}
	return -1;
}
//const string& proposer::find_key(const userid_t& id)
//{
//	for (int i = 1; i < (int)m_refs.size(); ++i)
//	{
//		if (m_refs[i].id == id)
//			return m_refs[i].key;
//	}
//	return m_refs[0].key;
//}
void proposer::on_timer_update_state()
{
	if (time_after(mtick(), m_icrr_begin_tick + m_timeout_ms))
	{
		//此时不要重置周期，避免误差积累
		set_state(IDLE);
		if (m_bmy)
		{
			blockmakerSngl::instance()->end_make_block();
			DEBUGMSG("%.1f: pro_endu\n", mtick() / (double)1000);
		}
		m_bmy = false;
	}
}
int proposer::begin_mypropose()
{
	reset_state(m_myindex, GIVING);
	m_bmy = true;
	DEBUGMSG("%.1f: pro_begin(%d)...\n", mtick()/(double)1000, m_myindex);
	return 0;
}

int proposer::add_vote(const userid_t& id,pc_rsp_propose_t& rsp)
{
	if (!m_bmy) return 1;

	for (list<pc_proposets_t>::iterator it = m_votes.begin(); it != m_votes.end(); ++it)
	{
		if ((*it).id == id)
			return  2;
	}

	//检查签名,确认块无过期
	hash_t hash;
	bstream s(1024);
	s << comSngl::instance()->id();
	s << rsp.timestamp;
	s << m_lbi.bt;
	sha3_256(hash.data(), 32, (uint8_t*)s.buffer(), s.length());
	if (!rsp.sign.empty() && cryp::sig::verify(id, rsp.sign, hash))
	{
		pc_proposets_t p;
		p.id = id;
		p.sign = rsp.sign;
		p.timestamp = rsp.timestamp;
		m_votes.push_back(p);
	}
	return 0;
}

int proposer::get_votes(pc_verify_propose_t& i)
{
	int k = 0;
	if (!m_bmy) return -1;
	i.height = m_lbi.bt.height;
	i.signs.resize(m_votes.size());
	for (list<pc_proposets_t>::iterator it= m_votes.begin(); it!=m_votes.end(); ++it)
	{
		i.signs[k++] = *it;
	}
	return 0;
}
int proposer::get_bsigns(pc_pub_blocksign_t& inf)const
{ 
	inf.bt.blockhash = m_myblock.blockhash;
	inf.bt.height = m_myblock.height;
	inf.vote_signs = m_bsigns; 
	return 0; 
}
void proposer::on_req_propose(const userid_t& id, const blocktag_t& req, pc_rsp_propose_t& rsp)
{
	//检：高度，周期，
	uint64_t tick = mtick();
	if (m_refs.empty())
	{
		rsp.result = -1;
		return;
	}
	//高度一致
	if (req != m_lbi.bt)
	{
		rsp.result = EPRO_HEIGHT;
		return;
	}
	//别人周期占用,假设同一ID同一周期不问两次
	if (m_icrrstate > IDLE && time_after(m_icrr_begin_tick + m_timeout_ms, tick))
	{
		rsp.result = EPRO_BUSY;
		return;
	}
	//未轮到,只要空闲就给票（不检排队)
	if (m_refs[(m_icrr + time_distance(tick, m_icrr_begin_tick) / m_timeout_ms) % m_refs.size()].id != id)
	{
		rsp.result = EPRO_ICRR;
		return;
	}

	//给
	//更新状态
	int i = find_index(id);
	if (i == -1)
	{
		rsp.result = EPRO_ID;
		return;
	}
	reset_state(i, GIVING);
	rsp.result = 0;
	//签名
	rsp.timestamp = mtick();
	hash_t hash;
	bstream s(1024);
	s << id;
	s << rsp.timestamp;
	s << req;
	sha3_256(hash.data(), 32, (uint8_t*)s.buffer(), s.length());
	cryp::sig::sign(comSngl::instance()->prikey(), hash, rsp.sign);
}
int proposer::on_verify_proposer(const userid_t& id, const pc_verify_propose_t& vp)
{
	int idx = find_index(id);
	if (-1 == idx)
		return 1;
	if (vp.height != m_lbi.bt.height)
		return 2;
	if (vp.signs.size() < (size_t)min_votes())
		return 3;
	//如果已给别人GIVEN，则此时不更新
	if (m_icrrstate >= GIVEN && time_after(m_icrr_begin_tick + m_timeout_ms, mtick()))
		return 4;
	
	//检查所有票签名
	hash_t hash;
	bstream s(1024);
	for (size_t i = 0; i < vp.signs.size(); ++i)
	{
		s.zero_rw();
		s << id;
		s << vp.signs[i].timestamp;
		s << m_lbi.bt;
		sha3_256(hash.data(), 32, (uint8_t*)s.buffer(), s.length());
		if (!cryp::sig::verify(vp.signs[i].id, vp.signs[i].sign, hash))
			return 5;
		//检自己签的时间戳
		if (vp.signs[i].id == comSngl::instance()->id())
		{
			uint64_t ti = time_distance(mtick(), vp.signs[i].timestamp);
			if (ti > (uint64_t)m_timeout_ms)
				return 6;
		}
	}
	//通过
	reset_state(idx, GIVEN);
	//DEBUGMSG("proposer::on_verify_proposer(%s) GIVEN\n", b2h(id).c_str());
	return 0;
}
int proposer::on_sync_proposer(const userid_t& id, uint64_t height)
{
	if (height == m_lbi.bt.height && m_refs[1].id == id && m_icrrstate < GIVEN && time_after(mtick(), m_last_have_given_tick + 2 * m_timeout_ms))
	{
		DEBUGMSG("on_sync_state\n");
		reset_state();
	}
	return 0;
}
int proposer::on_req_blocksign(const userid_t& id, cl::clslice& sb, pc_rsp_blocksign_t& r)
{
#define ereturn(i) {r.result = i;return i;}
	//检查是否为提议者并且投过票
	uint64_t tick = mtick();
	block_t b;
	bstream s((char*)sb.data(), (int)sb.size(), (int)sb.size());
	if (0 != s >> b || 0 != s.length())
		ereturn(1)

	r.height = b.height;
	//提过同一块，必须经两个周期才继续投
	if (b.height == last_give_blocksign_height && time_after(last_give_blocksign_tick + 2 * m_timeout_ms,tick))
		ereturn(2)
	int idx = find_index(id);
	//超时后其实m_icrrstate会被重置，即此处可不检超时
	if (m_icrr != idx || m_icrrstate != GIVEN || id != b.pub_out.id||time_after(tick,m_icrr_begin_tick+2*m_timeout_ms))
		ereturn(3);
	if (0!=chainSngl::instance()->check_block_ok(b, false)) //里面会计算hash
		ereturn(4);
	cryp::sig::sign(comSngl::instance()->prikey(), b.blockhash, r.sign);
	last_give_blocksign_height = b.height;
	last_give_blocksign_tick = tick;
	m_last_give_block = b;
	m_icrrstate = BLOCKSIGN_ING;
	//DEBUGMSG("on_req_blocksign() -> give block sign ok.");
	ereturn(0);
}
int proposer::handle_rsp_blocksign(const userid_t& id, const pc_rsp_blocksign_t& r)
{
	if (!m_bmy ||m_icrrstate!= BLOCKSIGN_ING || r.result != 0||r.height!=m_lbi.bt.height+1 || r.height!= m_myblock.height)
		return 1;
	signature_t s(id,r.sign);
	//if (m_bsigns.end() != std::find(m_bsigns.begin(), m_bsigns.end(),s))
	//	return 2;
	for (list<signature_t>::iterator it = m_bsigns.begin(); it != m_bsigns.end(); ++it)
	{
		if ((*it).id == id)
			return  2;
	}
	if (!cryp::sig::verify(id, r.sign, m_myblock.blockhash))
		return 3;
	m_bsigns.push_back(s);
	return 0;
}

