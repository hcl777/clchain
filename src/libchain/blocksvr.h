#pragma once
#include "common.h"
#include "tcppeer.h"
#include "cproto.h"

//œÏ”¶blockªÿ∏¥

class blocksvr : public tcppeerListener
{
public:
	blocksvr();
	virtual ~blocksvr();
	typedef struct {
		list<uint64_t> heights;
	}pdata_t;
public:
	int init();
	void fini();
	bool attach(tcppeer* peer);
	int rsp_height(tcppeer* peer);

	virtual void on(tcppeerListener::Disconnected, tcppeer* peer);
	virtual void on(tcppeerListener::Writable, tcppeer* peer);
	virtual void on(tcppeerListener::Packet, tcppeer* peer, uint8_t cmd, char* buf, int len);

private:
	list<tcppeer *> m_peers;
};
typedef singleton<blocksvr> blocksvrSngl;


