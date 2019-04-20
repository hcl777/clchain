#include "mci.h"
#include "cpo.h"
#include "blockmaker.h"
#include "referee.h"
#include "searcher.h"
#include "peersvr.h"
#include "blocksvr.h"
#include "blocksync.h"
#include "secdb.h"

enum MC_{
	MC_TRANS=1,
	MC_FIND_ASSETS
};

mci::mci()
{
}


mci::~mci()
{
}
int mci::init(const com_conf_t& c,bool bthread)
{
	int ret = 0;
	ret |= comSngl::instance()->init(c);
	ret |= chainSngl::instance()->init(c.wkdir+ DBNAME,c.cid);
	ret |= secdbSngl::instance()->init(c.wkdir + "sec.db");
	ret |= peersvrSngl::instance()->init();
	ret |= refereeSngl::instance()->init();
	ret |= blocksvrSngl::instance()->init();
	if(0==c.unsync)
		ret |= blocksyncSngl::instance()->init();

	ret |= blockmakerSngl::instance()->run(c.unbmkblock);
	ret |= searcherSngl::instance()->run(c.trackad);

	if (bthread) activate();
	return ret;
}
void mci::fini()
{
	m_brun = false;
	wait();
	searcherSngl::instance()->end();
	blockmakerSngl::instance()->end();

	blocksyncSngl::instance()->fini();
	blocksvrSngl::instance()->fini();
	refereeSngl::instance()->fini();
	peersvrSngl::instance()->fini();
	chainSngl::instance()->fini();
	secdbSngl::instance()->fini();
	comSngl::instance()->fini();

	searcherSngl::destroy();
	blockmakerSngl::destroy();
	blocksyncSngl::destroy();
	blocksvrSngl::destroy();
	refereeSngl::destroy();
	peersvrSngl::destroy();
	chainSngl::destroy();
	secdbSngl::destroy();
	comSngl::destroy();

	m_queue.clear(del_msg);
}
void mci::work(int e)
{
	while (m_brun)
		handle_loop();
}
void mci::del_msg(message *msg)
{
	delete msg;
}
void mci::handle_loop()
{
	uint64_t us;
	comSngl::instance()->timer.handle_root();
	us = comSngl::instance()->timer.get_remain_us();
	if (us > 30000) us = 30000;
	comSngl::instance()->reactor.handle_root(us);
	handle_message();
}
void mci::handle_message()
{
	message* msg = m_queue.tryget();
	if (!msg)return;
	switch (msg->cmd)
	{
	case MC_TRANS:
	{
		int *pret = (int*)msg->res;
		transaction_t *t = (transaction_t*)msg->data;
		*pret = blockmakerSngl::instance()->add_trans(*t);
		if(0==*pret) 
			refereeSngl::instance()->bloadcast_trans(*t);
		break;
	}
	case MC_FIND_ASSETS:
	{
		int *pret = (int*)msg->res;
		user_assets_t *i = (user_assets_t*)msg->data;
		*pret = chainSngl::instance()->find_utxo_assets(i->id,i->as);
		break;
	}
	default:
		assert(0);
		break;
	}
	del_msg(msg);
}
int mci::trans(transaction_t& t)
{
	int ret;
	if (!comSngl::instance()->is_referee())
		return -1;
	m_queue.send(new message(MC_TRANS, &t, &ret));
	return ret;
}
int mci::find_assets(user_assets_t& ua)
{
	int ret;
	m_queue.send(new message(MC_FIND_ASSETS, &ua, &ret));
	return ret;
}

