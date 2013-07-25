// main.cpp

// Standard C++ headers
#include <time.h>

// Local headers
#include "fuzzer.h"

using namespace std;

// Entry point
int main(int, char*[])
{
	const string application(
		"\"C:\\Program Files\\Autodesk\\Inventor 2012\\Bin\\Inventor.exe\"");
	const unsigned int maxExecutionTime(45 * 1000);// [msec]

	srand((unsigned int)time(NULL));

	// Not specifying execution limit -> run until killed
	Fuzzer fuzzer(application, maxExecutionTime);

	// TODO:  We'd probably rather specify a directory containing all of the
	// files to fuzz, in order to support large numbers of files
	fuzzer.AddTestFile("test1.ipt");
	fuzzer.AddTestFile("test2.ipt");
	fuzzer.AddTestFile("test3.iam");
	fuzzer.AddTestFile("test4.iam");
	fuzzer.AddTestFile("test5.idw");
	fuzzer.AddTestFile("test6.idw");

	fuzzer.Fuzz();

	return 0;
}