

#include <iostream>
#include "util.h"
#include "net.h"
#include "signature.h"
#include "jsonrpctest.h"
#include "jsonrp.h"

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

using namespace std;
using namespace cl;




void test()
{
	//cryp::sign_test();
	

	getchar();
}
int main(int argc, char** argv)
{
	//
	cout << "chain run..." << endl;
	cl::debug_memleak();
	cl::chang_dir(cl::get_module_dir().c_str());
	cl::net::init();

	test();
	while ('q' != getchar())
	{
		cl::msleep(1000);
	}

	cl::net::fini();
	return 0;
}

