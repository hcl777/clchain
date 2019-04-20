#pragma once
#include "common.h"
#include "chain.h"
#include "cproto.h"

/*
ͳ�ƿ�ͶƱ������
	cache block;state ״̬��
	��verifyʱ��֪ͨblockmaker����飬��1/3���ڶ�ʱ����ȡ��Ȼ��㲥ͶƱ����ʱ��verify��Ϊ׼��

�����߹�����
		��¼��������ʱ�䣬����ʱ��������¼��
	1����¼���һ�λ������Ȩ��verify ����ָ�Լ�)ʱ�䣬Լ��������3�������˻�ù�����Ȩ��
		���ɻ�Ա�����1�ŷ���һ��״̬ͬ�����յ�ͬ���ļ�鳬2���������˻�ù�����Ȩ�����á�
		
	2������������ᣬ���¸߶ȱ��(����״̬�����������Լ������ⲿ����
		ע�⣺��������ʱ�����ö�һ����������ͷ��������ÿ���һ�����鷢�����յ�һ��
		�������鷢�����ܣ���ע�������ط�ʹ��������ʱ�������1����
	3�����������Լ��Ƿ������ߣ��������������Լ�״̬�������Ʊ�����ݣ���
		A�ⲿ����ÿ1/10�������ڶ�ʱ����⣬Bÿ�θ��¸߶�ʱҲ��⣨������̸�᲻�죬
		��Ϊ����������ø��¸߶ȣ�����������ⲿ���ϼ���Ƿ��ֵ��Լ���
	4���Լ����������ڣ�
		�ṩ��ʼ����Ʊ״̬mybegin()���ռ���Ʊ�������ṩƱѡ���ȷ�ϡ�

		����״̬myend()����������������־bmy=faild��icrr���ָ���Լ�����state��Ϊidle��
	5���ṩ��ѯ��Ʊ�����
		���������bmyΪtrue����¡�
	6����Ӧreq_proposeЭ�鲢�ṩ�ظ������
		���һ���Լ�״̬�Ƿ��ڣ����ɽ����Լ���
		�ж϶Է��Ƿ��������ڣ��������
	7����Ӧverify_propose���ṩ�ظ������
		�յ�verify��������Լ�����������ڣ�������Լ�״̬(����>=given)�����ȸ�Ԥ��������Ȩ��
		���icrr״̬Ϊ>=given ����δ��ʱ������ԡ�ֻΪicrr�ṩǩ��Ȩ��
	8����Ӧreq_bvote���ṩ�ظ���������Ƿ�Ϊ����ͶƱ��
		icrr״̬Ϊ=given ����δ��ʱʱ���ų����ṩsign,��ɹ�����״̬��Ϊbsigned״̬��Ϊ��ǩ״̬����ʧ��ʱ��IDLE��
		���򷵻ؾ�ǩ��
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
	//��ʱ����Ƿ��ֵ��Լ�
	bool on_timer_check_mypropose_begin() {return (!m_bmy && m_myindex == (int)((m_icrr + time_distance(mtick(), m_icrr_begin_tick) / m_timeout_ms) % m_refs.size()));}
	int begin_mypropose();	
	//����Ƿ���Ҫͬ��״̬,��3�������˻������Ȩ
	bool on_timer_check_sync_state(){ 
		
		if (1 == m_myindex && m_icrrstate < GIVEN && time_after(mtick(), m_last_have_given_tick + 3 * m_timeout_ms))
		{
			m_last_have_given_tick = mtick(); //����ͬ����Ҳ����һ��
			return true;
		}
		return false;
	}
	int add_vote(const userid_t& id, pc_rsp_propose_t& rsp);
	//ʱ����2/3�����ڻ�ù�Ʊ��
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
	int min_votes()const { return (int)(m_refs.size() - 2) / 2 + 1; } //���룬��2Ϊ�Լ����׸��սڵ�

private:
	cpo_last_t			m_lbi;
	int					m_timeout_ms;
	int					m_myindex; //�ҵ��ź�

	vector<proinfo_t>	m_refs; //��Ա����0��Ϊ��

	//�ֺ�
	int					m_icrr; //�ֺţ���ǰ�ṩ�Ļ�����������ͶƱȨ��
	uint64_t			m_icrr_begin_tick; //�ֺſ�ʼʱ��
	int					m_icrrstate; //�ֺ�״̬��0=δ����1=�Ѹ���2=ȷ��
	uint64_t			m_last_have_given_tick; //������sync_state�жϣ���¼����ù�>=given��ʱ�䡣���ڼ���ͬ��

	//��¼����ǩ��Ϣ�����������ͬ���߶ȵĿ飬���뾭2���ں���Ͷ
	uint64_t			last_give_blocksign_tick;
	uint64_t			last_give_blocksign_height;
	block_t				m_last_give_block; //����ǩ��

	block_t					m_myblock; //���������block ,�ѼƵ�blockhash,��δһ����ǩ��
	bool					m_bmy;  //�ֵ��Լ�icrr=myindex
	list<pc_proposets_t>	m_votes;//propose ��Ʊ��.
	list<signature_t>		m_bsigns; //�������ǩ����
};

