#include "trahttp.h"
#include "util.h"
#include <iostream>
#include "clbase64.h"
#include "tracker.h"

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif



trahttp::trahttp()
{
#define REG_METHOD(func) m_reqfuncs[#func]=func;m_parser.register_request_callback(#func, func)
	REG_METHOD(version);
	REG_METHOD(get_node);
	REG_METHOD(update_directors);
	REG_METHOD(update_referees);
	REG_METHOD(report_refstate);
	REG_METHOD(search_referees);
}


trahttp::~trahttp()
{
}
int trahttp::init(uint16_t port, int threadnum)
{
	if (m_svr.is_open())
		return 1;
	if (threadnum < 1) threadnum = 1;
	if (threadnum > 1000) threadnum = 1000;
	if (0 != m_svr.open(port, NULL, on_request, this, true, threadnum, CHAIN_VERSION,false))
	{
		fini();
		return -1;
	}
	m_svrbak.open(port + 1, NULL, on_request, this, true, 2, CHAIN_VERSION, false);
	return 0;
}
void trahttp::fini()
{
	m_svr.stop();
	m_svrbak.stop();
}

int trahttp::on_request(HttpRequest_t* req)
{
	printf("# http_req: %s \n", req->cgi);
	trahttp* ph = (trahttp*)req->fun_param;
#define EIF(func) else if(strstr(req->cgi,#func)) ph->func(req)

	if (0 == strcmp(req->cgi, "/") || strstr(req->cgi, "/index."))
		ph->index(req);
	EIF(show_httpconf);
	EIF(state);
	EIF(jrapi);
	else
	{
		//目前只当他为文件
		string path = cl::get_module_dir();
		path += (req->cgi + 1);
		httprsp::response_file(req->fd, path);
	}

	return 0;
}
void trahttp::index(HttpRequest_t* req)
{
	char *buf = new char[20480];
#define CAT_TITLE(s) sprintf(buf+strlen(buf),"<br><strong><font>%s</font></strong><br>",s)
#define CAT(cgi) sprintf(buf+strlen(buf),"<a target=\"_blank\" href=\"%s\">%s</a><br>",cgi,cgi)

	sprintf(buf, "<h3>HTTP API(tracker):</h3>");
	CAT("/version");
	CAT("/myip");
	CAT("/show_httpconf");
	CAT("/state");
	CAT_TITLE("jsonrpc:");
	CAT("/jrapi");
	httprsp::response_message(req->fd, buf);
	delete[] buf;
}
void trahttp::show_httpconf(HttpRequest_t* req)
{
	const cl_httppubconf_t *c;
	char buf[1024];

	c = m_svr.get_conf();
	sprintf(buf, "<br>svr1:<br>"
		"ver=%s<br>"
		"begintime=%s<br>"
		"bmulti_thread=%d<br>"
		"bsupport_keepalive=%d<br>"
		"max_client_num=%d<br>"
		"current_clients=%d<br>"
		"request_amount=%d<br>"
		, c->ver.c_str(), c->begin_time.c_str(), c->multi_thread ? 1 : 0,
		c->bsupport_keepalive ? 1 : 0, c->max_client_num,
		c->client_num, c->request_amount);

	c = m_svrbak.get_conf();
	sprintf(buf + strlen(buf), "<br>svr2:<br>"
		"ver=%s<br>"
		"begintime=%s<br>"
		"bmulti_thread=%d<br>"
		"bsupport_keepalive=%d<br>"
		"max_client_num=%d<br>"
		"current_clients=%d<br>"
		"request_amount=%d<br>"
		, c->ver.c_str(), c->begin_time.c_str(), c->multi_thread ? 1 : 0,
		c->bsupport_keepalive ? 1 : 0, c->max_client_num,
		c->client_num, c->request_amount);
	httprsp::response_message(req->fd, buf, (int)strlen(buf));
}
void trahttp::state(HttpRequest_t* req)
{
	char buf[10240];
	track_state_t ts;
	trackerSngl::instance()->get_state(ts);
	xml_track_state(buf, ts);
	httprsp::response_message(req->fd, buf, (int)strlen(buf));
}
void trahttp::jrapi(HttpRequest_t* req)
{
	//req: {"jsonrpc":2.0,"method":"","params":[""],"id":1}
	//rsp: {"jsonrpc":2.0,"result":"","error":null,"id":1}
	string  s = "null", js;
	if (0 == stricmp(req->method, "get"))
		js = cl::url_get_param(req->params, "json");
	else
	{
		if (0 == httprsp::recv_body(req, js, 5000) && 0 == js.find("json="))
			js.erase(0, 5);
	}
	if (!js.empty())
	{
		//cout << "request:" << h << endl;
		jsonrpcpp::entity_ptr en;
		try{
			//这里用锁导致API编解码效率低
			//cl::Guard l(m_mt);
			//en = m_parser.parse(h.substr(5));
			//if (en->is_response())
			//	s = en->to_json().dump();
			//else
			//	throw jsonrpcpp::RpcException("unkown json-rpc request");
			
			//用局部变量避免被锁效率低
			jsonrpcpp::Parser parser;
			std::map<std::string, jsonrpcpp::request_callback>::iterator it;
			en = parser.parse(js);
			if(!en||!en->is_request())
				throw jsonrpcpp::RpcException("unkown json-rpc msg");
			jsonrpcpp::request_ptr re = dynamic_pointer_cast<jsonrpcpp::Request>(en);
			printf("jrapi method:%s  id=%d\n", re->method.c_str(),re->id.int_id); 
			it = m_reqfuncs.find(re->method);
			if (it != m_reqfuncs.end())
				s = it->second(re->id, re->params)->to_json().dump();
			else
				throw jsonrpcpp::RequestException(jsonrpcpp::Error("unkown json-rpc request",-1),re->id);
		}
		catch (const jsonrpcpp::RequestException& e)
		{
			s = e.to_json().dump();
		}
		catch (const jsonrpcpp::ParseErrorException& e)
		{
			s = e.to_json().dump();
		}
		catch (const jsonrpcpp::RpcException& e)
		{
			s = jsonrpcpp::ParseErrorException(e.what()).to_json().dump();
		}
		catch (const std::exception& e)
		{
			s = jsonrpcpp::ParseErrorException(e.what()).to_json().dump();
		}
	}
	else
	{
		s = jsonrpcpp::ParseErrorException(jsonrpcpp::Error("no json-rpc2.0 request!", -1)).to_json().dump();
	}

	httprsp::response_message(req->fd, s);
}

jsonrpcpp::response_ptr trahttp::version(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	return make_shared<jsonrpcpp::Response>(id, CHAIN_VERSION);
}
/*
随机返回一个记账节点的ip:httpport
*/
jsonrpcpp::response_ptr trahttp::get_node(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	uint32_t cid = trackerSngl::instance()->get_cid();
	nodeaddr_t n;
	char buf[1024];
	trackerSngl::instance()->get_rand_node(n);
	sprintf(buf, R"({"cid":%d,"svr":"http://%s/chain/jrapi"})",cid, net::htoas(n.pub.ip,n.hport).c_str());
	return make_shared<jsonrpcpp::Response>(id, buf);
}

#define CIDOK params.get<uint32_t>("cid")== trackerSngl::instance()->get_cid()

string& get_stream(string& out,const jsonrpcpp::Parameter& params)
{
	string s1;
	s1 = params.get<string>("stream"); 
	cl::base64::Decode(s1, &out);
	return out;
}

jsonrpcpp::response_ptr trahttp::update_directors(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	//在这里执行不能多线并发，效率低
	string s;
	int ret=-99;
	if(CIDOK)
		ret=trackerSngl::instance()->on_update_directors(get_stream(s, params));
	return make_shared<jsonrpcpp::Response>(id, ret);
}

jsonrpcpp::response_ptr trahttp::update_referees(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	//在这里执行不能多线并发，效率低
	string s;
	int ret = -99;
	if (CIDOK)
		ret = trackerSngl::instance()->on_update_referees(get_stream(s, params));
	return make_shared<jsonrpcpp::Response>(id, ret);
}
jsonrpcpp::response_ptr trahttp::report_refstate(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	//CALL_BBYTES(on_report_refstate)
	string s;
	int ret = -99;
	if (CIDOK)
		ret = trackerSngl::instance()->on_report_refstate(get_stream(s, params));
	return make_shared<jsonrpcpp::Response>(id, ret);
}

jsonrpcpp::response_ptr trahttp::search_referees(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	string s1, s2="";
	map<userid_t, nodeaddr_t> mp;
	//int ret = -99;
	if (CIDOK)
	{
		trackerSngl::instance()->search_referees(mp);
		cl::struct_to_string(mp, s1, 10240);
		cl::base64::Encode(s1, &s2);
	}
	return make_shared<jsonrpcpp::Response>(id, s2);
}

