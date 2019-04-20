
#include <iostream>

using namespace std;
using MYWORD = uint32_t;
int main(int argc, char** argv)
{
	char *p = nullptr;
	MYWORD w = 10255;
	cout << "test cmake!" << w << endl;

	p = new char[1024];

	delete[] p;
	cout << endl;
	return 0;

}
