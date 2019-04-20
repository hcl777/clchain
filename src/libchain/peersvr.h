#pragma once
#include "tcppeer.h"
#include "worker.h"
#include "cproto.h"
#include "tcpacceptor.h"
#include "singleton.h"

/*
创建维持peer连接池
。下载block，attache到blockserver处理数据回应
。申请
*/


class peersvr : public timerHandler
	,public tcpacceptorHandle
	,public tcppeerListener
{
public:
	peersvr();
	virtual ~peersvr();

public:
	int init();
	void fini();
	tcppeer* new_peer();
	void put_peer(tcppeer* peer);

	virtual void on_timer(int e);
	virtual bool attach_tcp_socket(int fd, sockaddr_in& addr);
	virtual void on(Disconnected, tcppeer* peer) { put_peer(peer);}
	virtual void on(tcppeerListener::Packet, tcppeer* peer, uint8_t cmd, char* buf, int len);
private:
	void pending_del();
private:
	tcpacceptor  m_tcpac;
	list<tcppeer*> m_peers,m_pending;
};

typedef singleton<peersvr> peersvrSngl;
