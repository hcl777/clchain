#pragma once
#include "tcppeer.h"
#include "proposer.h"

/*
ע����Ա�ı�����Ƿ��ڽ������棬��Ϊ����һ�ʽ��׷���������Ա������溬����ʱ�䣬
��ʾ��������Ҳ���Ըı䡣�����Ļ���Ҫ�����ԱԼ���Լ���������ҹ�ͬǩͬ������
������һ�����������ϡ�������ϸ��ϵ����ˣ����𲨡������Զ����ڱ����Ա��
��Ȼ����Ҳ����ͨ���Զ�ǩ�������ʽ��

referee�㷨��
	

	1������ִ�л�Ա���£��������
	2�����һ�Ա������ַ�����������ٲûụ����������Ӧ���Խ��������ٲû�attach
		��ʱ��ÿ����һ�����������Ƿ��������������
	3�����������ٲ�Э��
		���֣�У��ǩ����
	4������1/10���ڶ�ʱ����ʱ����proposer���Լ�����״̬��
	5��Ϊtracker�ṩ�ٲû�������ַ
	
*/
class referee : public timerHandler
	,public chainListener
	,public tcppeerListener
{
public:
	referee();
	virtual ~referee();
	enum PI_STATE{
		PI_IDLE=0,//δ��ͨ
		PI_CONN, //��ͨon_connteced
		PI_READY //����ͨ��
	};
	
	typedef struct tag_peerinfo {
		nodeaddr_t		n;
		tcppeer*		peer;
		int				state;
		tag_peerinfo() :peer(NULL), state(PI_IDLE) {}
	}peerinfo_t;
	typedef list<peerinfo_t*>	peerlist; //��ָ��
	typedef peerlist::iterator  peeriter;


	
public:
	int init();
	void fini();
	bool attach_peer(tcppeer* peer, endpoint_t& ep, userid_t& id);

	virtual void on_timer(int e);
	virtual void on(chainListener::UpdateReferee);
	virtual void on(chainListener::UpdateLBI);

	virtual void on(tcppeerListener::Connected, tcppeer* peer);
	virtual void on(tcppeerListener::Disconnected, tcppeer* peer);
	//virtual void on(tcppeerListener::Writable, tcppeer* peer) {}
	virtual void on(tcppeerListener::Packet, tcppeer* peer, uint8_t cmd, char* buf, int len);
	
	int on_handshake(tcppeer* peer, pc_handshake_t& inf);
	int bloadcast_trans(transaction_t& t);
private:
	void update_referees();
	void update_lbi();
	void free_all();
	void del_peer(peerinfo_t* pi);
	void put_peerinfo(peerinfo_t* pi) { del_peer(pi); delete pi; }
	peeriter find_pi(const userid_t& id);
	void ptl_req_propose();
	void ptl_verify_propose();
	void ptl_sync_propose();
	void ptl_req_blocksign(const string& b);
	void ptl_pub_blocksign();

private:
	peerlist			m_peers;
	int					m_iconn; //���ӳɹ���
	proposer			m_pp;
};
typedef cl::singleton<referee> refereeSngl;

