#include "trarun.h"
#include "util.h"
#include "net.h"
#include "timer.h"
#include "trahttp.h"
#include "tracker.h"
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif



void tra_print_help()
{
	printf("[ver]=%s\n", CHAIN_VERSION);
	printf("%s init params or %s params", cl::get_module_name().c_str(), cl::get_module_name().c_str());
	printf("[params]:\n");
	printf(">%-25s-- %s\n", "init <cid,genesisid>", "init track config.");
	printf(">%-25s-- %s\n", "-p <httpport>", "point to http port.");
	printf(">%-25s-- %s\n", "-n <threadnum>", "point to multi thread num.");
}
void tra_test()
{

}
int tra_main(int argc, char** argv)
{
	tra_test();

	int port = 5080;
	int i, ret, threadnum = 30;
	if (cl::string_array_find(argc, argv, "-h") > 0)
	{
		tra_print_help();
		return 0;
	}

	if ((i = cl::string_array_find(argc, argv, "-p")) > 0 && i + 1 < argc)
		port = (int)atoi(argv[i + 1]);
	if ((i = cl::string_array_find(argc, argv, "-n")) > 0 && i + 1 < argc)
		threadnum = (int)atoi(argv[i + 1]);
	if ((i = cl::string_array_find(argc, argv, "init")) > 0 && i + 1 < argc)
	{
		genesis_conf_t gc;
		string s = argv[i + 1];
		string s2;
		gc.cid = cl::atoi(cl::get_string_index(s, 0, ","), 0);
		gc.genesisid = cl::hexs2byte(cl::get_string_index(s, 1, ","), s2);
		gc.timestamp = time(NULL);
		if (0 == tracker::genesis_conf(gc))
			printf("genesis conf ok!\n");
		else
			printf("***genesis conf fail!\n");
		return 0;
	}

	cl::timerSngl::instance();
	ret = 0;
	ret |= trackerSngl::instance()->init();
	if (0 != ret)
	{
		printf("***track init failed! \n");
		goto end;
	}
	ret |= trahttpSngl::instance()->init(port, threadnum);

#ifdef _WIN32
	while ('q' != getchar())
#else
	while (1)
#endif
	{
		cl::timerSngl::instance()->handle_root();
		uint64_t ms = cl::timerSngl::instance()->get_remain_us();
		if (ms > 30000) ms = 30000;
		cl::msleep((uint32_t)(ms / 1000));
	}

end:
	trahttpSngl::instance()->fini();
	trackerSngl::instance()->fini();

	trahttpSngl::destroy();

	trackerSngl::destroy();
	cl::timerSngl::destroy();

	cl::net::fini();
	printf("end.\n");
	//getchar();
	return 0;
}
