#include "test_util.h"
#include "bit.h"
#include "bstream.h"
#include <iostream>

using namespace std;
using namespace cl;

void test_bit()
{
	uint32_t x = 0xeeeeffff;
	string s;
	cout << bit::to_string((uint8_t*)&x, 4, s) << endl;
}

void test_templatelist()
{
	bstream s(1024);
	list<int> ls, ls2;
	for (int i = 0; i<10; ++i)
		ls.push_back(i);
	s << ls;
	s >> ls2;
	for (list<int>::iterator it = ls2.begin(); it != ls2.end(); ++it)
	{
		cout << *it << endl;
	}
	getchar();
}

