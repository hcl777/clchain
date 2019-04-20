#pragma once
#include "chain.h"
#include "tcppeer.h"

/*
1. 定时搜索referee 源
2。至少随机连上3个源
3。连通时查询一次height,定时查询各自最新height
4。向chain获取未完整的cache任务单，分配下载。
*/

class blocksync : public timerHandler
	, public tcppeerListener
{
public:
	blocksync();
	virtual ~blocksync();
	enum PS_STATE {
		PS_IDLE = 0,
		PS_CONN,
		PS_READY
	};
	typedef struct tag_pdata {
		string			id;
		blocktag_t		bt;
		int				state = PS_IDLE;
		list<uint64_t>	ts;
	}pdata_t;
	typedef struct tag_task {
		list<tcppeer*>		ls;
	}task_t;

	typedef map<string,tcppeer*>		peerlist;
	typedef peerlist::iterator  peeriter;
public:
	int init();
	void fini();
	virtual void on_timer(int e);
	virtual void on(tcppeerListener::Connected, tcppeer* peer);
	virtual void on(tcppeerListener::Disconnected, tcppeer* peer);
	virtual void on(tcppeerListener::Packet, tcppeer* peer, uint8_t cmd, char* buf, int len);

private:
	void add_task(tcppeer* peer, list<uint64_t>& ls);
	void remove_task(tcppeer* peer, uint64_t height);
	void update_task();
	void assign_job(tcppeer* peer, pdata_t* pd);
	static int get_rand_addr(nodeaddr_t& n, map<userid_t, nodeaddr_t>& mp);
private:
	peerlist				m_peers;
	int						m_iconn;
	map<uint64_t, task_t>	m_ts;
	blocktag_t				m_bt;
};

typedef singleton<blocksync> blocksyncSngl;
