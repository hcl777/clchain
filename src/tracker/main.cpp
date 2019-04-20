
#include <iostream>
#include "util.h"
#include "net.h"
#include "timer.h"
#include "trarun.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif


int main(int argc, char** argv)
{
	cl::debug_memleak();
	cl::chang_dir(cl::get_module_dir().c_str());
	cl::net::init();

	return tra_main(argc, argv);
}

