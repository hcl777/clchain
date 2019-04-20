#include "searcher.h"
#include "httpc.h"
#include "trackproto.h"
#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

searcher::searcher()
	: m_last_getip_tick(0)
	, m_last_search_tick(0)
	, m_last_report_state_tick(0)
	, m_last_report_state_height(INVALID_HEIGHT)
	, m_last_update_directors_tick(0)
	, m_last_update_directors_height(INVALID_HEIGHT)
	, m_last_update_referees_tick(0)
	, m_last_update_referees_height(INVALID_HEIGHT)
{
}


searcher::~searcher()
{
}

int searcher::run(const string& svr)
{
	m_svr = svr;
	m_url_jrapi = "http://" + m_svr + "/jrapi";
	DEBUGMSG("#track:%s\n", m_svr.c_str());
	//comSngl::instance()->timer.register_timer(static_cast<timerHandler*>(this), 1, 1000);
	activate();
	return 0;
}
void searcher::end()
{
	//comSngl::instance()->timer.unregister_all(static_cast<timerHandler*>(this));
	m_brun = false;
	//m_msgq.post(new message(0, 0));
	wait();
	//m_msgq.clear(on_clear_msg);
}
void searcher::on_clear_msg(message* m)
{
	delete m;
}
void searcher::work(int e)
{
	uint64_t t = 0;
	while (m_brun)
	{
		msleep(300);
		t = mtick();
		if (!comSngl::instance()->is_referee())
		{
			//非会员只搜索源
			if ((zeroip_num(m_referees) > 0 && time_after(t, m_last_search_tick + 10000))
				|| time_after(t, m_last_search_tick + 60000))
			{
				search_referees();
			}
			continue;
		}

		if (0 == comSngl::instance()->na.pub.ip)
		{
			if (time_after(t,m_last_getip_tick + 10000))
			{
				if (0 == getip())
				{
					//上报一次最新信息
					msleep(100);
					update_directors();
					msleep(100);
					update_referees();
					msleep(100);
					report_refstate();
					msleep(100);
					search_referees();
				}
			}
			continue;
		}
		t = mtick();
		//上报状态，30秒必须报一次，防止tracker重启丢失数据时无效
		if (m_last_report_state_height != chainSngl::instance()->get_last_height()
			|| time_after(t, m_last_report_state_tick + 30000))
		{
			report_refstate();
		}

		//搜索源,每分种必须搜索一次，防止有变化
		if ((zeroip_num(m_referees) > 0 && time_after(t, m_last_search_tick + 10000))
			|| time_after(t, m_last_search_tick + 60000))
		{
			search_referees();
		}
		
		//上报地址变更
		if(m_last_update_directors_height !=chainSngl::instance()->get_directors_height()
			&& time_after(t, m_last_update_directors_tick + 10000))
			update_directors();
		if (m_last_update_referees_height != chainSngl::instance()->get_referees_height()
			&& time_after(t, m_last_update_referees_tick + 10000))
			update_referees();
	}
}

void searcher::on_timer(int e)
{
	switch (e)
	{
	case 1:
	{
		break;
	}
	default:
		assert(0);
		break;
	}
}
int searcher::get_node(nodeaddr_t& n)
{
	Guard l(m_mt_referees);
	map<userid_t, nodeaddr_t>::iterator it=m_referees.find(n.id);
	if (it != m_referees.end())
	{
		n = it->second;
		return 0;
	}
	return -1;
}
int searcher::get_referees(map<userid_t, nodeaddr_t>& mp)
{
	Guard l(m_mt_referees);
	mp = m_referees;
	return 0;
}
int searcher::getip()
{
	//DEBUGMSG("searcher::getip()...\n");
	m_last_getip_tick = mtick();
	string ip;
	string url = "http://" + m_svr + "/myip";
	if (0 != httpc::get(url, ip))
		return -1;
	comSngl::instance()->na.pub.ip = net::atoh(ip);
	//DEBUGMSG("searcher::getip() ok\n");
	return 0;
}
int searcher::update_directors()
{
	//DEBUGMSG("searcher::update_directors()...\n");
	tran_names_t na;
	int ret;
	m_last_update_directors_tick = mtick();
	if (0 == post_jrapi_bbytes_sign("update_directors", chainSngl::instance()->get_directors(na), ret))
	{
		m_last_update_directors_height = na.height;
		//DEBUGMSG("searcher::update_directors() ok\n");
		return 0;
	}
	return -1;
}
int searcher::update_referees()
{
	//DEBUGMSG("earcher::update_referees()...\n");
	tran_names_t na;
	int ret;
	m_last_update_referees_tick = mtick();
	if (0 == post_jrapi_bbytes_sign("update_referees", chainSngl::instance()->get_referees(na), ret))
	{
		//暂只要服务器返回结果就不再发，避免客户端判断自己为会员，tracker却不认为的矛盾反复上报。
		m_last_update_referees_height = na.height;
		//DEBUGMSG("earcher::update_referees() ok\n");
		return 0;
	}
return -1;
}
int searcher::report_refstate()
{
	//DEBUGMSG("searcher::report_refstate()...\n");
	tp_state_t ti;
	int ret;

	m_last_report_state_tick = mtick();
	ti.na = comSngl::instance()->na;
	chainSngl::instance()->get_last_blockinfo(ti.lbi);
	if (0 == post_jrapi_bbytes_sign("report_refstate", ti, ret))
	{
		m_last_report_state_height = ti.lbi.bt.height;
		//DEBUGMSG("searcher::report_refstate() ok\n");
		return 0;
	}
	return -1;
}
int searcher::search_referees()
{
	//DEBUGMSG("searcher::search_referees()...\n");
	m_last_search_tick = mtick();
	string s1,s2;
	map<userid_t, nodeaddr_t> mp;
	jsonrpcpp::Parser parser;
	jsonrpcpp::entity_ptr en;
	
	char buf[1024];
	sprintf(buf, R"(json={"jsonrpc":"2.0","method":"search_referees","params":{"cid":%d},"id":1})",
		chainSngl::instance()->cid());
	if (0 == httpc::post(m_url_jrapi, clslice(buf,strlen(buf)), s2))
	{
		en = parser.parse(s2);
		if (en&&en->is_response())
		{
			jsonrpcpp::response_ptr re = dynamic_pointer_cast<jsonrpcpp::Response>(en);
			s1 = re->result.get<string>();
			cl::base64::Decode(s1, &s2);
			cl::string_to_struct(s2, mp);
			if (!mp.empty())
			{
				DEBUGMSG("searcher::search_referees(%d) ok\n", (int)mp.size());
				Guard l(m_mt_referees);
				m_referees.swap(mp);
				
			}
		}
	}
	return 0;
}
int searcher::zeroip_num(map<userid_t, nodeaddr_t>& mp)
{
	int num = 0;
	map<userid_t, nodeaddr_t>::iterator it;
	for (it = mp.begin(); it != mp.end(); ++it)
	{
		if (it->second.pub.ip == 0)
			num++;
	}
	return num;
}
