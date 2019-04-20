#include "test_sha.h"
#include <iostream>
#include "util.h"
#include "sha1.h"
#include "clsha3.h"
#include <string.h>
#include <list>
#include "p2pproto.h"
#include <algorithm>

void test_sha()
{
	int len = 20480 * 1024;
	char *buf = new char[len];
	unsigned char hash[64];
	std::string strhash;
	uint64_t tick;

	memset(buf, 0xfe, len);
	printf("sha: bufsize=%d\n", len);
	while (1)
	{

		cl::msleep(100);
		tick = cl::mtick();
		cl::sha3_256(hash, 32, (uint8_t*)buf, len);
		cl::byte2hexs(hash, 32, strhash);
		std::cout << "sha3:" << cl::mtick() - tick << strhash << std::endl;

		tick = cl::mtick();
		cl::sha1_buffer(hash, 20, (uint8_t*)buf, len);
		cl::byte2hexs(hash, 20, strhash);
		std::cout << "sha1:" << cl::mtick() - tick << strhash << std::endl;

		cl::msleep(500);
	}
	delete[] buf;
}

void test_clhash_list()
{
	using namespace std;
	using namespace p2p;
	list<nodeid_t> ls;
	list<nodeid_t>::iterator it;
	nodeid_t id;
	int i;
	string str;
	for (i = 0; i < 10; ++i)
	{
		ls.push_back(id.rand());
		cout << id.to_hexString(str) << endl;
	}
	i = 0;
	for (it = ls.begin(); it != ls.end(); ++it)
	{
		if (++i == 5)
		{
			id = *it;
			break;
		}
	}
	it = ls.end();

	it = find(ls.begin(), ls.end(), id);
	if (it != ls.end())
	{
		cout << "id  :" << endl << id.to_hexString(str) << endl;
		cout << "find:" << endl << (*it).to_hexString(str) << endl;
	}
	getchar();
}
