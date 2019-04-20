#pragma once
#include "common.h"
#include "message.h"
#include "worker.h"
#include "block.h"

//message call interface

#define DBNAME "chain"

class mci : public worker
{
public:
	mci();
	virtual ~mci();

	int init(const com_conf_t& c,bool bthread);
	void fini();
	virtual void work(int e);
	static void del_msg(message *msg);
	void handle_loop();

	int trans(transaction_t& t);
	int find_assets(user_assets_t& ua);
private:
	void handle_message();
private:
	msgQueue m_queue;

};

typedef cl::singleton<mci> mciSngl;
