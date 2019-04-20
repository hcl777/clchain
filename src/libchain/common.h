#pragma once
#include "cltype.h"
#include "incnet.h"
#include "timer.h"
#include "clreactor.h"
#include "singleton.h"
#include "memblock.h"
#include "clhash.h"
#include "guards.h"
//#include "p2pproto.h"
#include "bstream.h"
#include "signature.h"


//using namespace p2p;
using namespace std;
using namespace cl;
using namespace cryp;
//using namespace p2p;
//using userid_t = p2p::nodeid_t;

using hash_t = cl::h256;

//直接使用pubkey做ID更高效，pubkey为定长33
//using userid_t = cl::h256;
using userid_t = string;
#define IDSIZE 32

#define CHAIN_VERSION "CHAIN_20190327"

enum MEMBLOCK_SIZE {
	MB1K = 1024,
	MB2K = 2048,
	MB1M = 1 << 20
};



typedef struct tag_nodeaddr {
	//以下ID使用二进制保存
	userid_t		id;//节点KAD网ID,也是应用账号ID，会随应用层变更而变更
	endpoint_t		pub; //外网
	endpoint_t		local; //内网,TCP只使用local中的port
	uint8_t			nat_type=0;
	uint16_t		hport=0;//http端口
	//bool operator ==(const tag_nodeaddr& n)const { return n.id == id; }
}nodeaddr_t;

typedef struct tag_ecuser {
	string pubk;
	string prik;
	tag_ecuser to_hexs()const
	{
		tag_ecuser u;
		cl::byte2hexs(prik, u.prik);
		cl::byte2hexs(pubk, u.pubk);
		return u;
	}
	tag_ecuser to_bytes()const
	{
		tag_ecuser u;
		cl::hexs2byte(prik, u.prik);
		cl::hexs2byte(pubk, u.pubk);
		return u;
	}
}ecuser_t;

typedef struct tag_com_conf {
	string		wkdir;
	uint32_t	cid;
	ecuser_t	us;
	string		trackad; //ipport
	endpoint_t	local;
	endpoint_t	pub; //外部可指定可不指定
	uint16_t	httpport;
	int			unbmkblock = 0; //不生产
	int			unsync = 0;
	string		aeskey; //用于rpc json执行交易请求的加密传递user prikey接口
}com_conf_t;

int operator << (bstream& s, const nodeaddr_t& i);
int operator >> (bstream& s, nodeaddr_t& i);

int operator << (bstream& s, const ecuser_t& i);
int operator >> (bstream& s, ecuser_t& i);

class comm
{
	friend class cl::singleton<comm>;
private:
	comm();
	~comm();
public:
	int init(const com_conf_t& conf);
	void fini();
	void set_is_referee(bool b)  { m_is_referee = b; }

	bool is_referee() const { return m_is_referee; }
	const userid_t& id()const { return m_id; }
	const string& prikey()const { return m_prikey; }
	bool is_me(const userid_t& id)const { return (m_id == id); }

	static int user_make(ecuser_t& u);
	static int user_save(const string& path,const string& ak,const ecuser_t& u);
	static int user_load(const string& path, const string& ak, ecuser_t& u);
public:
	cl::reactorSelect	reactor;
	cl::timer			timer;
	cl::memblockPool	mempool;
	nodeaddr_t			na;
	string				wkdir;
private:
	userid_t			m_id;
	string				m_prikey; //用于投票签名
	bool				m_is_referee;
};
typedef cl::singleton<comm> comSngl;

