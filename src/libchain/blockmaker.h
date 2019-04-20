#pragma once
#include "common.h"
#include "worker.h"
#include "cpo.h"


/*
���������߹��ܣ�
	1����¼���н��׷Ž��������list1,map1,
	2���������߳���֤���׺ʹ���鹤����
		��ʹ�ö��߳�����Ϊ����chain��������¶��߳������塣
		����ʱ��list1�Ľ��׷Ž�map2�б���list1ʱ��ȥ�غ���������
		����map2ʱ��key=timestamp+tran_hash����ʱ�����С�
		ÿ����һ��list1���ף����ѯһ���Ƿ��д���������������ִ�д��������
		������õĿ�ִ��һ�ζ�ʱ��תΪ���߳��ύ��referee��
	3�����մ���������´������״̬��
	4�����ʱ����¼���׽���map2��ȡ��map2�Ľ��׵�chain��鲻����ǩ�����Ч�ʡ�
	�յ�һ����httpAPI�������Ľ���ʱ����Ҫͨ���ٲû�㲥��������Ա��
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

	int add_trans(const transaction_t& t); //���ٲû��յ��㲥
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
