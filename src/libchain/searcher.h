#pragma once

#include "common.h"
#include "worker.h"
#include "chain.h"
#include "cproto.h"
#include "message.h"
#include "clbase64.h"
#include "jsonrp.h"
#include "guards.h"
#include "httpc.h"
#include "trackproto.h"

/*
searcher���ܣ�
	�ϱ�״̬������lbi�͵�ַ��Ϣ��
	1.����ʱ����ȡ�Լ�����IP��ֱ����ȡ�ɹ�����ȡ�ɹ����ϱ�һ������״̬��
	2��ÿ�θ���lbiʱ�ϱ�һ������״̬��
	3����һ��last_report_ok ��¼�Ƿ��ϱ��ɹ�����ʱ�������ϱ�ʧ�ܣ����ظ��ϱ���

	1������ʱ�ͱ���ٲû�����ʱ����Լ��ǻ�Ա���ϱ���ַ�������tracker
	������ڵ�ַ��Ĭ��д���ڳ����ڣ�Ҳ�����ⲿָ������
		�ɿ��Ǵ�referee�л�ȡ������Ա��ַһ���ϱ����ࡣ
		��Ա����߱�����IP���Լ���IPֱ��ͨ��������ѯ�õ���Ҳ���Լ�ָ���������С�
		��Աͨ�����������ȡ�Լ���ַIPPORT�����IPΪ0������ͨ��ϵͳȡ������ַIP����
		����ͬ�������ɲ��ԣ��ϱ���ַ��������IP��
	2������ʱ�����ٲû�����ʱ������Լ��ǻ�Ա�������ٲû��ַ��
		��������Ϊ�ṩ�ٲû�����ID,��tracker���ض�ӦID�ĵ�ַ��Ϣ��
		��ѯʱ����鲻ȫ����ÿ��1���ظ���ѯ��
	ע�����������ܲ���ͬһ���߳�ִ�У��̶߳��ڼ�����������״̬�Ƿ���ɣ�δ������ظ���
		�ϱ���ַ��������Ա���������������ݽṹ����ָ�
	3���ṩ�ӿڸ�referee��ѯ���л�Ա��ַ��

*/

class searcher : public worker
	,public timerHandler
{
public:
	searcher();
	virtual ~searcher();
public:
	int run(const string& svr);
	void end();
	static void on_clear_msg(message* m);

	virtual void work(int e);
	virtual void on_timer(int e);

	int get_node(nodeaddr_t& n);
	int get_referees(map<userid_t, nodeaddr_t>& mp);

private:
	int getip();
	int update_directors();
	int update_referees();
	int report_refstate();
	int search_referees();
	static int zeroip_num(map<userid_t, nodeaddr_t>& mp);

	template<typename TPARAMS,typename TRESULT>
	int post_jrapi_bbytes_sign(const string& method, TPARAMS& pa, TRESULT& ret)
	{
		string s1, s2;
		cl::struct_to_string(pa, s1, 1024);
		tp_sign(comSngl::instance()->prikey(), comSngl::instance()->id(), s1, s2);
		base64::Encode(s2, &s1);

		s2 = R"(json={"jsonrpc":"2.0","method":")" + method + R"(","params":{"cid":)"+
			chainSngl::instance()->cids() + R"(,"stream":")" + s1 + R"("},"id":1})";
		if (0 == httpc::post(m_url_jrapi, s2, s1))
		{
			try {
				jsonrpcpp::Parser parser;
				jsonrpcpp::entity_ptr en;
				jsonrpcpp::response_ptr re;
				en = parser.parse(s1);
				if (en&&en->is_response())
				{
					jsonrpcpp::response_ptr re = dynamic_pointer_cast<jsonrpcpp::Response>(en);
					ret = re->result.get<TRESULT>();
					return 0;
				}
			}
			catch (...)
			{
				return -2;
			}
		}
		return -1;
	}

private:
	//msgQueue					m_msgq;
	string						m_svr;
	string						m_url_jrapi;
	Mutex						m_mt_referees;
	map<userid_t, nodeaddr_t>	m_referees;
	uint64_t					m_last_getip_tick;
	uint64_t					m_last_search_tick;
	uint64_t					m_last_report_state_tick;
	uint64_t					m_last_report_state_height;
	uint64_t					m_last_update_directors_tick;
	uint64_t					m_last_update_directors_height;
	uint64_t					m_last_update_referees_tick;
	uint64_t					m_last_update_referees_height;
};
typedef cl::singleton<searcher> searcherSngl;
