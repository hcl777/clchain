#include "conf.h"
#include "inifile.h"
#include "net.h"
#include "util.h"

#define DF_CID 0xcc11ffee
#define DF_TRACKAD ""
#define DF_HTTPPORT 10068
#define DF_CPORT 10255
/*
*/
int load_conf(const string& wkdir, com_conf_t& c)
{
#define se "public"
	inifile i;
	char buf[1024];
	string s;
	if (0 == i.open((wkdir + "conf.ini").c_str()))
	{
		c.cid = i.read_int(se, "cid", 0);
		c.httpport = (uint16_t)i.read_int(se, "httpport", 0);
		c.local.port = (uint16_t)i.read_int(se, "cport", 0);
		c.trackad = i.read_string(se, "trackad","", buf, 1024);
		s = i.read_string(se, "us", "", buf, 1024);
		c.us.pubk = get_string_index(s, 0, "@");
		c.us.prik = get_string_index(s, 1, "@");
		s = i.read_string(se, "pub", "", buf, 1024);
		c.pub.set(net::atoh(get_string_index(s, 0, ":")), (uint16_t)cl::atoi(get_string_index(s, 1, ":"), 0));

		c.unbmkblock = i.read_int(se, "unmkblock", 0);
		c.unsync = i.read_int(se, "unsync", 0);
		c.aeskey = i.read_string(se, "ekey","", buf,1024);
	}
	c.wkdir = wkdir;
	c.local.ip = net::atoh(net::get_local_private_ip());
	if (0 == c.local.port) c.local.port = DF_CPORT;
	if (0 == c.pub.port) c.pub.port = c.local.port;
	if (0 == c.httpport) c.httpport = DF_HTTPPORT;
	if (c.trackad.empty()) c.trackad = DF_TRACKAD;
	if (0 == c.cid) c.cid = DF_CID;
	DEBUGMSG("local ip:%s\n", net::htoas(c.local.ip).c_str());
	return 0;
}
