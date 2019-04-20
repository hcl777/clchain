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
searcher功能：
	上报状态（包括lbi和地址信息）
	1.启动时，获取自己外网IP，直至获取成功。获取成功后上报一次最新状态。
	2。每次更新lbi时上报一次最新状态。
	3。用一个last_report_ok 记录是否上报成功，定时检查如果上报失败，则重复上报。

	1。启动时和变更仲裁会名单时如果自己是会员则上报地址到接入点tracker
	（接入节地址点默认写死在程序内，也可以外部指定）。
		可考虑从referee中获取其它会员地址一起上报冗余。
		会员必须具备公网IP，自己的IP直接通过网卡查询得到，也可自己指定到配置中。
		会员通过向接收器获取自己地址IPPORT（如果IP为0，则再通过系统取本机地址IP）。
		考虑同局域网可测试，上报地址包括内网IP。
	2。启动时或变更仲裁会名单时，如果自己是会员则搜索仲裁会地址。
		搜索过程为提供仲裁会所有ID,让tracker返回对应ID的地址信息。
		查询时如果查不全，将每隔1秒重复查询。
	注：以上两功能采用同一条线程执行，线程定期检查上面的任务状态是否完成，未完成则重复。
		上报地址和搜索会员用两个独立的数据结构保存指令。
	3。提供接口给referee查询所有会员地址。

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
