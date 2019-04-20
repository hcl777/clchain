
#include <iostream>
#include "util.h"
#include "net.h"
#include "mci.h"
#include <algorithm>
#include "signature.h"
#include "httpap.h"
#include "chainview.h"
#include "conf.h"
#include "trarun.h"

#include "crypp.h"
#include "clbase64.h"
#include "base64.h"


#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

using namespace std;

template<typename K, typename V>
int testT(std::map<K, V>& i)
{
	K k;
	V v;
	i.clear();
	for (int j = 0; j < 5; j++)
	{
		k = cl::hash<32>::rand_hash().to_hexString();
		v = j;
		i[k] = v;
	}
	return 0;
}

int test()
{
	std::map<string, int> mp;
	//testT(mp);
	//mp[h2b("25086560cc4c6d33a36f355fd9c558044d93620284920a5d5b841ee6d95b018d")] = 1;
	//mp[h2b("3d8c0497c1a0857a3397a1e8e3f506845774a08bb06bcd42204f349bfa2c4aac")] = 2;
	//for (std::map<string, int>::iterator it = mp.begin(); it != mp.end(); ++it)
	//{
	//	printf("%s : %d \n", it->first.c_str(), it->second);
	//}
	//return -1;
	return 0;
}

void print_help()
{
	string ename = cl::get_module_name();
	printf("[ver]=%s\n", CHAIN_VERSION);
	printf("%s user <params...>\n", ename.c_str());
	printf("%s genesis\n", ename.c_str());
	printf("%s view\n", ename.c_str());
	printf("%s tra\n", ename.c_str());
	printf("%s <params...>\n", ename.c_str());
	printf("[params]:\n");
	printf(" >%-25s-- %s\n", "--path", "workdir(default: [module_dir]/data).");
	printf("[user params]:\n");
	printf(" >%-25s-- %s\n", "user --make", "make new user.");
	printf(" >%-25s-- %s\n", "user --see <id>", "see user(id) ok key .");
	printf(" >%-25s-- %s\n", "user --testmake", "loop test make new user and sign.");
	printf(" >%-25s-- %s\n", "user --test <id> <pass>", "test user sign.");
	printf(" >%-25s-- %s\n", "user --testw <id> <pass>", "test user sign wrong(switch key or wrong prikey).");
}

int main_user(int argc, char** argv, int i,const string& wkdir)
{
	ecuser_t u,u2;
	char pass[1024], pass2[1024];
	string path;
	int ret;
	if (i + 1 >= argc)
	{
		printf("[params]:\n--make\n--see <id>\n--testmake\n--test <id> <pass>"
			"\n--testw <id> <pass>\n");
		return -1;
	}
	if (0 == strcmp(argv[i + 1], "--make"))
	{
		if (0 == comSngl::instance()->user_make(u))
		{
			printf("new id: [%s]\n", u.pubk.c_str());
			for (int j = 0; j<3; ++j)
			{
				printf("enter password(2~16):");
				get_scanf_pass(pass, 1024);
				printf("enter password again:");
				get_scanf_pass(pass2, 1024);
				size_t len = strlen(pass);
				path = wkdir + "user/" + u.pubk + ".usk";
				if (len>1 && len<17 && 0 == strcmp(pass, pass2)
					&& 0 == comSngl::instance()->user_save(path, pass, u))
				{
					printf(">make user ok! remember your pass and id:\n[%s]\n", u.pubk.c_str());
					printf(">please backup your user file:\n[%s]\n", path.c_str());
					return 0;
				}
			}
		}
		printf("***make user wrong!\n");
		return 0;
	}
	else if (0 == strcmp(argv[i + 1], "--see"))
	{
		if (i + 2 < argc)
		{
			path = wkdir + "user/" + argv[i + 2] + ".usk";
			for (int j = 0; j < 3; ++j)
			{
				printf("enter password:");
				get_scanf_pass(pass, 1024);
				ret = comSngl::instance()->user_load(path, pass, u);
				if (-1 == ret)
				{
					printf("***no user file:[%s]\n", path.c_str());
					return 0;
				}
				else if (0 == ret)
				{
					printf("user(id=pubkey):\n");
					printf("pubkey:[%s]\n", u.pubk.c_str());
					printf("prikey:[%s]\n", u.prik.c_str());
					return 0;
				}
			}
		}
		return 0;
	}
	else if (0 == strcmp(argv[i + 1], "--testmake"))
	{
		hash_t hash;
		string sign;
		while (1)
		{
			if (0 == comSngl::instance()->user_make(u2))
			{
				printf("user:\n");
				printf("pubkey:[%s]\n",u2.pubk.c_str());
				printf("prikey:[%s]\n", u2.prik.c_str());

				cl::hexs2byte(u2.prik, u.prik);
				cl::hexs2byte(u2.pubk, u.pubk);
				hash.rand();
				cryp::sig::sign(u.prik, hash, sign);
				if (cryp::sig::verify(u.pubk, sign, hash))
					printf("test sign ok:[%s]\n", hash.to_hexString().c_str());
				else
					printf("***test sign fail:[%s]\n", hash.to_hexString().c_str());
			}
			msleep(500);
		}
	}
	else if (0 == strcmp(argv[i + 1], "--test"))
	{
		//测试正确签名
		if (i + 3 > argc)
			return 0;
		path = wkdir + "user/" + argv[i + 2] + ".usk";
		string pa = argv[i + 3];
		ecuser_t u2;
		if (0 != comSngl::instance()->user_load(path, pa, u2))
			return 0;
		hash_t hash;
		string sign;
		cl::hexs2byte(u2.prik, u.prik);
		cl::hexs2byte(u2.pubk, u.pubk);

		while(1)
		{
			hash.rand();
			cryp::sig::sign(u.prik, hash, sign);
			if (cryp::sig::verify(u.pubk, sign, hash))
				printf("test sign ok:[%s]\n", hash.to_hexString().c_str());
			else
				printf("***test sign fail:[%s]\n", hash.to_hexString().c_str());
			msleep(500);
		}
		return 0;
	}
	else if (0 == strcmp(argv[i + 1], "--testw"))
	{
		//测试错误签名
		if (i + 3 > argc)
			return 0;
		path = wkdir + "user/" + argv[i + 2] + ".usk";
		string pa = argv[i + 3];
		ecuser_t u2;
		if (0 != comSngl::instance()->user_load(path, pa, u2))
			return 0;
		hash_t hash;
		string sign;
		int n = 0;
		cl::hexs2byte(u2.prik, u.prik);
		cl::hexs2byte(u2.pubk, u.pubk);

		while (1)
		{
			hash.rand();
			if (n++ % 2 == 0)
			{
				cryp::sig::sign(u.pubk, hash, sign);
				if (cryp::sig::verify(u.prik, sign, hash))
					printf("test sign1 ok:[%s]\n", hash.to_hexString().c_str());
				else
					printf("***test sign1 fail:[%s]\n", hash.to_hexString().c_str());
			}
			else
			{
				string k = u.prik;
				k.at(rand() % 32) = (char)rand();
				cryp::sig::sign(k, hash, sign);
				if (cryp::sig::verify(u.pubk, sign, hash))
					printf("test sign2 ok:[%s]\n", hash.to_hexString().c_str());
				else
					printf("***test sign2 fail:[%s]\n", hash.to_hexString().c_str());
			}
			
			
			msleep(500);
		}
		return 0;
	}

	return 0;
}
int main(int argc, char** argv)
{
	//cout << "chain run..." << endl;
	cl::debug_memleak();
	cl::chang_dir(cl::get_module_dir().c_str());
	cl::net::init();

	if (0 != test())return -1;

	int i,ret=0;
	string wkdir = cl::get_module_dir()+"data/";
	com_conf_t c;
	string dbpath;

	if(2==argc && 0==strcmp(argv[1],"-h"))
	{
		print_help();
		return 0;
	}
	if(argc>1 && 0==strcmp(argv[1],"tra"))
		return tra_main(argc, argv);

	//1.先取工作路经，user/genesis/view 等需要使用
	if ((i = cl::string_array_find(argc, argv, "--path")) > 0 && i + 1 < argc)
	{
		wkdir = argv[i + 1];
		cl::dir_format(wkdir);
	}
	//cl::chang_dir(wkdir.c_str());
	dbpath = wkdir + DBNAME;

	//2.user/genesis/view
	//账号
	if ((i = cl::string_array_find(argc, argv, "user")) > 0)
		return main_user(argc, argv, i, wkdir);
	if ((i = cl::string_array_find(argc, argv, "genesis")) > 0)
	{
		chainSngl::instance()->init(dbpath,0);
		if (0 != chainSngl::instance()->genesis(wkdir+"genesis.json"))
			printf("*** genesis fail!\n");
		else
			printf("genesis ok!\n");
		chainSngl::destroy();
		return 0;
	}
	if ((i = cl::string_array_find(argc, argv, "view")) > 0)
		return chain_view(dbpath);

	//3.常规运行
	load_conf(wkdir,c);
	if (c.trackad.empty())
	{
		cout << "*** trackad empty!" << endl;
		return 0;
	}
	c.us = c.us.to_bytes();
	
	//检查密码
	char pass[1024];
	string prikey;
	int n = 0;
	do
	{
		if (sig::check_key(c.us.prik, c.us.pubk))
			break;
		printf("id:%s\n", b2h(c.us.pubk).c_str());
		printf("enter your prikey:");
		get_scanf_pass(pass, 1024);
		c.us.prik = h2b(pass);
	} while (++n < 3);

	if (!sig::check_key(c.us.prik, c.us.pubk))
	{
		cout << "*** user wrong!\n";
		return 0;
	}

	ret |= mciSngl::instance()->init(c, true);
	ret |= httpapSngl::instance()->init(c.httpport,c.aeskey, 20);
	if (0 != ret) goto end;

#ifdef _WIN32
	while ('q' != getchar())
	{
		msleep(500);
	}
#else
	while (1) msleep(1000);
#endif // _WIN32
end:

	httpapSngl::instance()->fini();
	mciSngl::instance()->fini();

	httpapSngl::destroy();
	mciSngl::destroy();

	cl::net::fini();
	return 0;
}

