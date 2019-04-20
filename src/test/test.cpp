

#include <iostream>
#include "util.h"
#include "net.h"
#include "sha1.h"
#include "bit.h"
#include "clhash.h"
#include "host.h"
#include "test_util.h"
#include "chain.h"
#include <ctime>
#include <iomanip>

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

using namespace std;
using namespace p2p;
using namespace cl;

void test_p2p();
void test()
{
	std::time_t t = std::time(nullptr);
	
	std::cout << "UTC:       " << std::put_time(std::gmtime(&t), "%c %Z") << '\n';
	std::cout << "local:     " << std::put_time(std::localtime(&t), "%c %Z") << '\n';
	// POSIX-specific:
	std::string tz = "TZ=Asia/Singapore";
	putenv(tz.data());
	std::cout << "Singapore: " << std::put_time(std::localtime(&t), "%c %Z") << '\n';
}
int main(int argc, char** argv)
{
	
	
	cl::debug_memleak();
	cl::chang_dir(cl::get_module_dir().c_str());
	cl::net::init();

	//test:
	test();
	//test_p2p();
	
	cl::net::fini();
	getchar();
	return 0;
}
void test_p2p()
{
	p2p::hostSngl::instance()->init(20550, p2p::nodeid_t::rand_hash(), 123, true);

	while ('q' != getchar())
		cl::msleep(100);

	p2p::hostSngl::instance()->fini();
	p2p::hostSngl::destroy();
}


