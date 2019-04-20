#pragma once
#include "common.h"
#include "worker.h"
#include "cpo.h"


/*
区块制作者功能：
	1。收录所有交易放进待检队列list1,map1,
	2。独立单线程验证交易和打包块工作。
		不使用多线程是因为访问chain会加锁导致多线程无意义。
		空闲时检list1的交易放进map2列表，检list1时仅去重和验命名。
		保存map2时用key=timestamp+tran_hash。按时序排列。
		每检完一笔list1交易，则查询一次是否有打包块命令，有则立即执行打包工作。
		将打包好的块执行一次定时器转为主线程提交给referee。
	3。接收打包工作更新打包命令状态。
	4。打包时，收录交易仅从map2获取，map2的交易调chain检查不再验签名提高效率。
	收到一个从httpAPI发过来的交易时，需要通过仲裁会广播给其它成员。
*/

class blockmaker : public worker
{
public:
	blockmaker();
	virtual ~blockmaker();

public:
	int run(int unmkblock);
	void end();
	virtual void work(int e);

	int add_trans(const transaction_t& t); //从仲裁会收到广播
	void begin_make_block();
	int try_get_block(block_t& b);
	void end_make_block();
private:
	void thread_tran();
	void thread_block();
	static int make_block(block_t& b,ecuser_t& u, map<string, transaction_t*>& mp);
	
	void test_make_block();
private:
	Mutex							m_mtls, m_mtmp,m_mtblock;
	int								m_make_block;
	block_t							m_block;
	list<transaction_t*>			m_ls;
	map<string, transaction_t*>		m_mp;
	bool							m_unmkblock;
};

typedef cl::singleton<blockmaker> blockmakerSngl;
