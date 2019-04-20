#pragma once

#include <singleton.h>
#include <httpsvr.h>
#include "jsonrp.h"

class httpap
{
public:
	httpap();
	~httpap();

	int init(uint16_t port, const string& ekey, int threadnum);
	void fini();

	static int on_request(HttpRequest_t* req);

private:
	void index(HttpRequest_t* req);
	void show_httpconf(HttpRequest_t* req);
	void jrapi(HttpRequest_t* req);	
	
	static jsonrpcpp::response_ptr version(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr new_user(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr trans_test(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr token_test(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr names_test(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr get_trans_stamp(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr trans(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr get_trans_state(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr trans2(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr get_assets(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr get_flow(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
private:
	httpsvr m_svr, m_svrbak;
	std::map<std::string, jsonrpcpp::request_callback> m_reqfuncs;
	string m_strfunc;
	static string m_aeskey;

};
typedef cl::singleton<httpap> httpapSngl;

