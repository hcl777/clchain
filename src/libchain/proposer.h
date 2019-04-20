#pragma once
#include "common.h"
#include "chain.h"
#include "cproto.h"

/*
统计块投票方案：
	cache block;state 状态；
	当verify时，通知blockmaker打包块，设1/3周期定时，获取块然后广播投票，计时以verify的为准。

提议者工作：
		记录提议周期时间，周期时间由链记录。
	1。记录最后一次获得提议权（verify 不仅指自己)时间，约定当超过3周期无人获得过提议权，
		则由会员排序第1号发起一次状态同步。收到同步的检查超2个周期无人获得过提议权即重置。
		
	2。被动更新议会，更新高度变更(重置状态，包括结束自己），外部调用
		注意：更新名单时，设置多一个空名单在头部，用于每完成一个区块发布都空等一周
		避免区块发布过密，但注意其它地方使用名单表时需减掉第1个。
	3。被动计算自己是否提议者，是则马上设置自己状态（清理计票器内容）。
		A外部采用每1/10提议周期定时器检测，B每次更新高度时也检测（更新座谈会不检，
		因为随后立即调用更新高度），计算完后外部马上检测是否轮到自己。
	4。自己提议周期内：
		提供初始化计票状态mybegin()，收集计票，被动提供票选结果确认。

		结束状态myend()：不作数据清理，标志bmy=faild，icrr如果指向自己，则state置为idle。
	5。提供查询计票结果：
		结果必须在bmy为true情况下。
	6。响应req_propose协议并提供回复结果。
		检查一次自己状态是否超期，超由结束自己。
		判断对方是否在周期内，是则给。
	7。响应verify_propose并提供回复结果。
		收到verify，如果是自己提议的周期内，则结束自己状态(不管>=given)，优先给预别人提议权。
		如果icrr状态为>=given 并且未超时，则忽略。只为icrr提供签名权。
	8。响应req_bvote并提供回复结果（即是否为区块投票）
		icrr状态为=given 并且未超时时，才尝试提供sign,验成功并将状态置为bsigned状态即为已签状态，验失败时置IDLE。
		否则返回拒签。
*/


enum EPRO_ {
	EPRO_HEIGHT =1,
	EPRO_BUSY,
	EPRO_ICRR,
	EPRO_ID
};
class referee;
class proposer
{
	friend class referee;
public:
	typedef struct tag_proinfo
	{
		userid_t		id;
		//string			key;
	}proinfo_t;

	enum STATE{IDLE=0,GIVING,GIVEN,BLOCKSIGN_ING};
	proposer();
	~proposer() {}
public:
	void set_state(int i) { 
		if (m_icrrstate>GIVING && i< GIVEN)
			m_last_have_given_tick = mtick(); //
		m_icrrstate = i;
	}
	void reset_state(int icrr = 0, int state = IDLE);
	void update_referees(map<userid_t, int>& r);
	void update_lbi();
	int get_timeout_ms()const {return m_timeout_ms;}
	void on_timer_update_state();
	//定时检查是否轮到自己
	bool on_timer_check_mypropose_begin() {return (!m_bmy && m_myindex == (int)((m_icrr + time_distance(mtick(), m_icrr_begin_tick) / m_timeout_ms) % m_refs.size()));}
	int begin_mypropose();	
	//检查是否需要同步状态,超3周期无人获得提议权
	bool on_timer_check_sync_state(){ 
		
		if (1 == m_myindex && m_icrrstate < GIVEN && time_after(mtick(), m_last_have_given_tick + 3 * m_timeout_ms))
		{
			m_last_have_given_tick = mtick(); //发起同步后也重置一下
			return true;
		}
		return false;
	}
	int add_vote(const userid_t& id, pc_rsp_propose_t& rsp);
	//时间在2/3周期内获得够票数
	bool is_votes_ok(){ return  (m_bmy &&GIVING==m_icrrstate && time_after(m_icrr_begin_tick + m_timeout_ms * 2 / 3, mtick()) && m_votes.size() >= (size_t)min_votes()); }
	int get_votes(pc_verify_propose_t& i);
	int get_bsigns(pc_pub_blocksign_t& inf)const;
	bool is_mygiven()const { return (m_bmy&&m_icrrstate == GIVEN); }
	bool is_blocksign_ok() { return (m_bmy&&BLOCKSIGN_ING == m_icrrstate && m_bsigns.size() >= chainSngl::instance()->min_vote_num()); }
	

	void on_req_propose(const userid_t& id, const blocktag_t& req, pc_rsp_propose_t& rsp);
	int on_verify_proposer(const userid_t& id, const pc_verify_propose_t& vp);
	int on_sync_proposer(const userid_t& id, uint64_t height);
	int on_req_blocksign(const userid_t& id, cl::clslice& sb, pc_rsp_blocksign_t& r);
	int handle_rsp_blocksign(const userid_t& id, const pc_rsp_blocksign_t& r);
private:
	int find_index(const userid_t& id);
	//const string& find_key(const userid_t& id);
	
	void sort_ids();
	int min_votes()const { return (int)(m_refs.size() - 2) / 2 + 1; } //过半，减2为自己和首个空节点

private:
	cpo_last_t			m_lbi;
	int					m_timeout_ms;
	int					m_myindex; //我的排号

	vector<proinfo_t>	m_refs; //会员，第0个为空

	//轮号
	int					m_icrr; //轮号，当前提供的或满足条件获投票权，
	uint64_t			m_icrr_begin_tick; //轮号开始时间
	int					m_icrrstate; //轮号状态，0=未给，1=已给，2=确定
	uint64_t			m_last_have_given_tick; //仅用于sync_state判断，记录最后获得过>=given的时间。用于计算同步

	//记录最后给签信息，如果有人提同样高度的块，必须经2周期后再投
	uint64_t			last_give_blocksign_tick;
	uint64_t			last_give_blocksign_height;
	block_t				m_last_give_block; //最后给签的

	block_t					m_myblock; //我最提议的block ,已计得blockhash,但未一定有签名
	bool					m_bmy;  //轮到自己icrr=myindex
	list<pc_proposets_t>	m_votes;//propose 得票数.
	list<signature_t>		m_bsigns; //提议块获得签名数
};

