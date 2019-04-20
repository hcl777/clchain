#pragma once
#include "tcppeer.h"
#include "proposer.h"

/*
注：议员的变更考虑放在交易里面，作为其中一笔交易发表变更。议员变更里面含周期时间，
表示出块周期也可以改变。这样的话需要链外会员约定自己变更，并且共同签同意变更后，
再像交易一样发布到链上。这就是严格上的联盟，如瑞波。不能自动定期变更会员。
当然后期也可以通过自动签发变更方式。

referee算法：
	

	1。被动执行会员更新，最后块更新
	2。查找会员名单地址，建立所有仲裁会互联，包括响应来自接收器的仲裁会attach
		定时器每秒检查一次所有连接是否断线重连工作。
	3。处理所有仲裁协议
		握手：校验签名，
	4。设置1/10周期定时器定时调度proposer查自己提议状态。
	5。为tracker提供仲裁会名单地址
	
*/
class referee : public timerHandler
	,public chainListener
	,public tcppeerListener
{
public:
	referee();
	virtual ~referee();
	enum PI_STATE{
		PI_IDLE=0,//未连通
		PI_CONN, //连通on_connteced
		PI_READY //握手通过
	};
	
	typedef struct tag_peerinfo {
		nodeaddr_t		n;
		tcppeer*		peer;
		int				state;
		tag_peerinfo() :peer(NULL), state(PI_IDLE) {}
	}peerinfo_t;
	typedef list<peerinfo_t*>	peerlist; //存指针
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
	int					m_iconn; //连接成功数
	proposer			m_pp;
};
typedef cl::singleton<referee> refereeSngl;

