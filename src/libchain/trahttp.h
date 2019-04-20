#pragma once
#include <singleton.h>
#include <httpsvr.h>
#include "jsonrp.h"
#include "guards.h"


class trahttp
{
public:
	trahttp();
	~trahttp();

	int init(uint16_t port,int threadnum);
	void fini();

	static int on_request(HttpRequest_t* req);

private:
	void index(HttpRequest_t* req);
	void show_httpconf(HttpRequest_t* req);
	void state(HttpRequest_t* req);
	void jrapi(HttpRequest_t* req);

	static jsonrpcpp::response_ptr version(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr get_node(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr update_directors(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr update_referees(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr report_refstate(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
	static jsonrpcpp::response_ptr search_referees(const jsonrpcpp::Id& id, const jsonrpcpp::Parameter& params);
private:
	httpsvr m_svr, m_svrbak;

	std::map<std::string, jsonrpcpp::request_callback> m_reqfuncs;
	cl::Mutex		m_mt;
	jsonrpcpp::Parser m_parser;
};

typedef cl::singleton<trahttp> trahttpSngl;
