#include "httpap.h"
#include "util.h"
#include <iostream>
#include "clbase64.h"
#include "common.h"
#include "chain.h"
#include "mci.h"
//#include "aes128.h"
#include "crypp.h"
#include "secdb.h"

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif


string httpap::m_aeskey = "";
httpap::httpap()
{
	m_strfunc = "[jsonrpc_method]:<br>";
#define REG_METHOD(func) m_reqfuncs[#func]=func; m_strfunc+="---"#func"<br>"
	REG_METHOD(version);
	REG_METHOD(new_user);
	REG_METHOD(trans_test);
	REG_METHOD(token_test);
	REG_METHOD(names_test);
	REG_METHOD(get_trans_stamp);
	REG_METHOD(trans);
	REG_METHOD(get_trans_state);
	REG_METHOD(trans2); 
	REG_METHOD(get_assets);
	REG_METHOD(get_flow);
}


httpap::~httpap()
{
}
int httpap::init(uint16_t port,const string& ekey, int threadnum)
{
	if (m_svr.is_open())
		return 1;
	if (threadnum < 1) threadnum = 1;
	if (threadnum > 1000) threadnum = 1000;
	m_aeskey = ekey;
	if (0 != m_svr.open(port, NULL, on_request, this, true, threadnum, CHAIN_VERSION, false))
	{
		fini();
		return -1;
	}
	m_svrbak.open(port + 1, NULL, on_request, this, true, 2, CHAIN_VERSION, false);
	return 0;
}
void httpap::fini()
{
	m_svr.stop();
	m_svrbak.stop();
}

int httpap::on_request(HttpRequest_t* req)
{
	printf("# http_req: %s \n", req->cgi);
	httpap* ph = (httpap*)req->fun_param;
#define EIF(func) else if(strstr(req->cgi,"/chain/"#func)) ph->func(req)

	if (0 == strcmp(req->cgi, "/") || strstr(req->cgi, "/index"))
		ph->index(req);
	EIF(show_httpconf);
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
void httpap::index(HttpRequest_t* req)
{
	char *buf = new char[20480];
#define CAT_TITLE(s) sprintf(buf+strlen(buf),"<br><strong><font>%s</font></strong><br>",s)
#define CAT(cgi) sprintf(buf+strlen(buf),"<a target=\"_blank\" href=\"%s\">%s</a><br>",cgi,cgi)

	sprintf(buf, "<h3>HTTP API(tracker):</h3>");
	CAT("/version");
	CAT("/chain/show_httpconf");
	CAT_TITLE("jsonrpc:");
	CAT("/chain/jrapi");
	sprintf(buf + strlen(buf), "%s", m_strfunc.c_str());
	httprsp::response_message(req->fd, buf);
	delete[] buf;
}
void httpap::show_httpconf(HttpRequest_t* req)
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

void httpap::jrapi(HttpRequest_t* req)
{
	//req: {"jsonrpc":"2.0","method":"","params":{},"id":1}
	//rsp: {"jsonrpc":"2.0","result":"","error":null,"id":1}
	string  s="null", js;
	if (0 == stricmp(req->method, "get"))
		js = cl::url_get_param(req->params, "json");
	else
	{
		if (0 == httprsp::recv_body(req, js, 5000) && 0 == js.find("json="))
			js.erase(0, 5);
	}
	if (!js.empty())
	{
		jsonrpcpp::entity_ptr en;
		try {
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
			if (!en->is_request())
				throw jsonrpcpp::RpcException("unkown json-rpc msg");
			jsonrpcpp::request_ptr re = dynamic_pointer_cast<jsonrpcpp::Request>(en);
			DEBUGMSG("[jrapi_method]--%s\n", re->method.c_str());
			it = m_reqfuncs.find(re->method);
			if (it != m_reqfuncs.end())
				s = it->second(re->id, re->params)->to_json().dump();
			else
				throw jsonrpcpp::RequestException(jsonrpcpp::Error("unkown json-rpc request", -1), re->id);
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

jsonrpcpp::response_ptr httpap::version(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	return make_shared<jsonrpcpp::Response>(id, CHAIN_VERSION);
}
jsonrpcpp::response_ptr httpap::new_user(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	char buf[1024];
	ecuser_t u;
	comSngl::instance()->user_make(u);
	sprintf(buf, R"({"id":"%s","key":"%s"})", u.pubk.c_str(), u.prik.c_str());
	return make_shared<jsonrpcpp::Response>(id, buf);
}
jsonrpcpp::response_ptr httpap::trans_test(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	//{"from":"id@prikey","to":"id","assets":123,"fee",2}
	//收成交易单提交
	int ret=-1;
	char buf[1024] = { 0, };
string s;
printf("jsonrpc::[trans_test]:%s\n", params.to_json().dump().c_str());
ecuser_t u;
string tokid;
uint64_t fee, in_height;
uint32_t cid;
vector<tran_out_t>	outs;
tran_out_t ta;
string ext;
transaction_t t;

s = params.get<string>("from");
u.pubk = cl::get_string_index(s, 0, "@");
u.prik = cl::get_string_index(s, 1, "@");
//支持token 转账
tokid = cl::h2b(params.get<string>("tokid"));

ta.id = cl::h2b(params.get<string>("to"));
ta.asset = params.get<uint64_t>("assets");
outs.push_back(ta);

ext = params.get<string>("ext");
////test:
//ta.id = cl::h2b("61a7f432857b553a25de015e1cb045378db27c534d57efd6decca2b75056c99e");
//ta.asset = 3000;
//outs.push_back(ta);

//2b
u = u.to_bytes();

fee = chainSngl::instance()->get_set().min_tranfee;
in_height = chainSngl::instance()->get_last_height() + 1;
cid = chainSngl::instance()->cid();

ret = trans::new_tran(t, cid, u, tokid, fee, in_height, outs, ext);
if (0 == ret)
{
	ret = mciSngl::instance()->trans(t); //-1失败，0等待结果
	if (0 == ret)
	{
		sprintf(buf, "%s#%llu", t.tranhash.to_hexString().c_str(), (ull)t.data.in_height);
	}
}
return make_shared<jsonrpcpp::Response>(id, buf);
}

jsonrpcpp::response_ptr httpap::token_test(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	int ret = -1;
	char buf[1024] = { 0, };
	ecuser_t u, dire;
	string s, name, description;
	uint64_t assets = 0;
	transaction_t t;
	uint64_t fee, in_height;
	uint32_t cid;

	s = params.get<string>("from");
	u.pubk = cl::get_string_index(s, 0, "@");
	u.prik = cl::get_string_index(s, 1, "@");
	u = u.to_bytes();
	assets = params.get<uint64_t>("assets");
	name = params.get<string>("name");
	description = params.get<string>("description");

	dire.pubk = comSngl::instance()->id();
	dire.prik = comSngl::instance()->prikey();
	in_height = chainSngl::instance()->get_last_height() + 1;
	cid = chainSngl::instance()->cid();
	fee = chainSngl::instance()->get_set().min_consfee;

	if (0 == trans::new_token(t, cid, u, dire, in_height, assets, name, description, fee))
	{
		ret = mciSngl::instance()->trans(t);
		if (0 == ret)
			sprintf(buf, "%s#%llu@%s", t.tranhash.to_hexString().c_str(), (ull)t.data.in_height, t.data.tok.hash.to_hexString().c_str());
	}


	return make_shared<jsonrpcpp::Response>(id, buf);
}
jsonrpcpp::response_ptr httpap::names_test(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	int ret = -1;
	char buf[1024] = { 0, };
	ecuser_t u;
	string s;
	transaction_t t;
	uint64_t in_height;
	uint32_t cid;
	tran_names_t ns;
	uint8_t type = 0;

	s = params.get<string>("from");
	u.pubk = cl::get_string_index(s, 0, "@");
	u.prik = cl::get_string_index(s, 1, "@");
	u = u.to_bytes();
	type = params.get<uint8_t>("type");
	jsonrpcpp::Parameter  pn = params.get("names");
	int i = 0;
	ns.names.resize(pn.param_array.size());
	for (const auto& n : pn.param_array)
	{
		ns.names[i++] = n.get<string>();
	}
	if(type == TRT_DIRECTOR)
		ns.height = chainSngl::instance()->get_directors_height();
	else if(type == TRT_REFEREE)
		ns.height = chainSngl::instance()->get_referees_height();
	else
		throw jsonrpcpp::RequestException(jsonrpcpp::Error("wrong type", -1), id);

	if(ns.names.empty())
		throw jsonrpcpp::RequestException(jsonrpcpp::Error("wrong names", -2), id);

	in_height = chainSngl::instance()->get_last_height() + 1;
	cid = chainSngl::instance()->cid();

	if (0 == trans::new_names(t, cid, u, in_height, ns,type))
	{
		ret = mciSngl::instance()->trans(t);
		if (0 == ret)
			sprintf(buf, "%s#%llu", t.tranhash.to_hexString().c_str(), (ull)t.data.in_height);
		else
			throw jsonrpcpp::RequestException(jsonrpcpp::Error("add trans error", -3), id);
	}


	return make_shared<jsonrpcpp::Response>(id, buf);
}

jsonrpcpp::response_ptr httpap::get_trans_stamp(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	char buf[1024];
	sprintf(buf, R"({"cid":%d,"in_height":%llu,"timestamp":%llu})",
		chainSngl::instance()->cid(), (ull)chainSngl::instance()->get_last_height()+1, (ull)time(NULL));
	return make_shared<jsonrpcpp::Response>(id, buf);
}

jsonrpcpp::response_ptr httpap::trans(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	string s1, s2; 
	//char buf[1024] = { 0, };
	int ret=-2; 
	transaction_t t;
	s1 = params.get<string>("stream"); 
	cl::base64::Decode(s1, &s2); 
	if (0 == string_to_struct(s2, t))
	{
		ret = mciSngl::instance()->trans(t); //-1失败，0等待结果
	}
	//此时不返回交易单ID，1是没有执行运算hash,2是不检查具体交易类型，
	//3是用调用方自己可根本需要组交易相关ID
	return make_shared<jsonrpcpp::Response>(id, ret);
}
jsonrpcpp::response_ptr httpap::get_trans_state(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	//hash#in_height
	string s1, s2;
	int ret;
	s1 = params.get<string>("sn");
	ret = chainSngl::instance()->find_tran(cl::get_string_index(s1, 0, "#"), cl::atoll(cl::get_string_index(s1, 1, "#").c_str()));
	return make_shared<jsonrpcpp::Response>(id, ret);
}
jsonrpcpp::response_ptr httpap::trans2(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	//加密的交易请求
	char buf[1024] = { 0, };
	string s,s1, s2; 
	ecuser_t u;
	string tokid;
	uint64_t fee, in_height;
	uint32_t cid;
	vector<tran_out_t>	outs;
	tran_out_t ta;
	string ext;
	transaction_t t;
	int ret;

	s1 = params.get<string>("stream"); 
	cl::base64::Decode(s1, &s2); 
	if(0!=cl::aescbc::decrypt_s(httpap::m_aeskey, s2, s1))
		return make_shared<jsonrpcpp::Response>(id, "");
	
	//from@key,to,tokid,asset,ext
	jsonrpcpp::Parameter par;
	par.parse(s1);

	s = par.get<string>("from");
	u.pubk = cl::get_string_index(s, 0, "@");
	u.prik = cl::get_string_index(s, 1, "@");
	//支持token 转账
	tokid = cl::h2b(params.get<string>("tokid"));

	ta.id = cl::h2b(params.get<string>("to"));
	ta.asset = params.get<uint64_t>("assets");
	outs.push_back(ta);

	ext = params.get<string>("ext");
	u = u.to_bytes();
	fee = chainSngl::instance()->get_set().min_tranfee;
	in_height = chainSngl::instance()->get_last_height() + 1;
	cid = chainSngl::instance()->cid();
	if (0 == trans::new_tran(t, cid, u, tokid, fee, in_height, outs, ext))
	{
		ret = mciSngl::instance()->trans(t); //-1失败，0等待结果
		if (0 == ret)
		{
			sprintf(buf, "%s#%llu", t.tranhash.to_hexString().c_str(), (ull)t.data.in_height);
		}
	}
	return make_shared<jsonrpcpp::Response>(id, buf);
}
jsonrpcpp::response_ptr httpap::get_assets(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	char buf[1024];
	user_assets_t ua;
	string uid,tokid;
	uid = params.get<string>("id");
	tokid = params.get<string>("tokid");
	ua.id = h2b(uid)+h2b(tokid);
	mciSngl::instance()->find_assets(ua);
	sprintf(buf, R"({"assets":%llu,"height":%llu,"flag":%d})",
		(ull)ua.as.assets, (ull)ua.as.height,(int)ua.as.flag);
	return make_shared<jsonrpcpp::Response>(id, buf);
}
jsonrpcpp::response_ptr httpap::get_flow(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params)
{
	string uid, beg_date, end_date;
	string sret;
	uid = params.get<string>("id");
	beg_date = params.get<string>("beg_date");
	end_date = params.get<string>("end_date");
	secdbSngl::instance()->get_flow(sret, uid, beg_date, end_date);
	return make_shared<jsonrpcpp::Response>(id, sret.c_str());
}