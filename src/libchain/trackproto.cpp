#include "trackproto.h"
#include "signature.h"
#include "util.h"
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#define xml_begin(xmlbuf) sprintf(xmlbuf,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<root>\r\n")
#define xml_end(xmlbuf) sprintf(xmlbuf+strlen(xmlbuf),"</root>")


//在头部增加一个signature_t
int tp_sign(const string& prikey, const string& pubkey, const string& data, string& out)
{
	signature_t sig;
	sig.id = pubkey;
	cryp::sig::sign(prikey, cl::clslice(data), sig.sign);
	cl::struct_to_string(sig, out, 1024);
	out += data;
	return 0;
}
//检查签名是否OK=0
int tp_unsign(const string& s, cl::clslice& out_data, signature_t& sig)
{
	string str;
	bstream b((char*)s.data(), (int)s.size(), (int)s.size());
	if (0 == b >> sig)
	{
		out_data.set(b.read_ptr(), b.length());
		return 0;
	}
	return -1;
}

int operator << (bstream& s, const genesis_conf_t& i)
{
	s << i.cid;
	s << i.genesisid;
	s << i.timestamp;
	return s.ok();
}
int operator >> (bstream& s, genesis_conf_t& i)
{
	s >> i.cid;
	s >> i.genesisid;
	s >> i.timestamp;
	return s.ok();
}

int operator << (bstream& s, const tp_state_t& i)
{
	s << i.na;
	s << i.lbi;
	s << i.timestamp;
	return s.ok();
}
int operator >> (bstream& s, tp_state_t& i)
{
	s >> i.na;
	s >> i.lbi;
	s >> i.timestamp;
	return s.ok();
}
void xml_add_list(char* xmlbuf, const char* idname, list<string>& ls)
{
	sprintf(xmlbuf + strlen(xmlbuf), "	<list id=\"%s\" len=\"%d\">\r\n ", idname, (int)ls.size());
	for (list<string>::iterator it = ls.begin(); it != ls.end(); ++it)
		sprintf(xmlbuf + strlen(xmlbuf), "		<p>%s</p>\r\n", (*it).c_str());
	sprintf(xmlbuf + strlen(xmlbuf), "	</list>\r\n ");
}
void  xml_track_state(char* xmlbuf, track_state_t& ts)
{
	string s1, s2;
	xml_begin(xmlbuf);
	sprintf(xmlbuf + strlen(xmlbuf),
R"(	<p n="begin_time">%s</p>
	<seg id="genesis_conf">
		<p n="cid">%d</p>
		<p n="genesisid">%s</p>
		<p n="init_time">%s</p>
	</seg>
)"
	, cl::time_to_datetime_string(ts.begintime).c_str()
	,ts.gc.cid, cl::byte2hexs(ts.gc.genesisid,s1).c_str(), cl::time_to_datetime_string(ts.gc.timestamp).c_str());


	list<string> dirs, refs;
	map<userid_t, tp_state_t>::iterator it;
	char buf[1024];
	s1 = "id|(pub,local,hport)|(height,hash,timestamp)|(last_alive_time)";
	dirs.push_back(s1);
	refs.push_back(s1);
	for (it = ts.dirs.begin(); it != ts.dirs.end(); ++it)
	{
		tp_state_t& i = it->second;
		sprintf(buf, "%s|(%s,%s,%d)|(%lld,%s,%s)|(%s)"
			, cl::byte2hexs(i.na.id, s1).c_str()
			, net::htoas(i.na.pub.ip, i.na.pub.port).c_str()
			, net::htoas(i.na.local.ip, i.na.local.port).c_str()
			, (int)i.na.hport, (long long)i.lbi.bt.height
			, i.lbi.bt.blockhash.to_hexString().c_str()
			, cl::time_to_datetime_string(i.lbi.timestamp).c_str()
			, cl::time_to_datetime_string(i.timestamp).c_str()
		);
		dirs.push_back(buf);
	}
	for (it = ts.refs.begin(); it != ts.refs.end(); ++it)
	{
		tp_state_t& i = it->second;
		sprintf(buf, "%s|(%s,%s,%d)|(%lld,%s,%s)|(%s)"
			, cl::byte2hexs(i.na.id, s1).c_str()
			, net::htoas(i.na.pub.ip, i.na.pub.port).c_str()
			, net::htoas(i.na.local.ip, i.na.local.port).c_str()
			, (int)i.na.hport,(long long) i.lbi.bt.height
			, i.lbi.bt.blockhash.to_hexString().c_str()
			, cl::time_to_datetime_string(i.lbi.timestamp).c_str()
			, cl::time_to_datetime_string(i.timestamp).c_str()
		);
		refs.push_back(buf);
	}
	xml_add_list(xmlbuf, "directors", dirs);
	xml_add_list(xmlbuf, "referees", refs);
	xml_end(xmlbuf);
}



