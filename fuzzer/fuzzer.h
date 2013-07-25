// fuzzer.h

#ifndef FUZZER_H_
#define FUZZER_H_

// Standard C++ headers
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

// MS headers
#include <Windows.h>

using namespace std;

class Fuzzer
{
public:
	Fuzzer(const string &application, const unsigned int &maxExecutionTime,
		const unsigned int &maxExecutions = 0, const unsigned int &maxBitsToFlip = 256);

	void AddTestFile(const string &file);
	void Fuzz(void);

	inline bool IsGood(void) const { return files.size() > 0; };
	inline bool IsFinished(void) const {
		return !IsGood() || (test >= maxExecutions && maxExecutions > 0); };

private:
	const string application;
	const unsigned int maxExecutionTime;
	const unsigned int maxExecutions;
	const unsigned int maxBitsToFlip;

	vector<string> files;
	unsigned int test;
	unsigned int crashCount;

	stringstream logBuffer;

	void RunNextTest(void);

	// This structure permits communication between the main thread and the
	// process-launching threads
	struct ProcessInfo
	{
		HANDLE processHandle;
		HANDLE startSemaphore;
		HANDLE endSemaphore;

		int returnValue;
		volatile bool done;
		string command;
	} pi;

	bool SetupNextTest(void);

	string ChooseAndCopyRandomInputFile(void);

	bool FlipRandomBits(const string &fileName);
	static void FlipBit(char *buffer, const unsigned int &bit);

	static void LaunchTimedExecution(void *args);

	void WriteCaseToLogFile(const int &returnValue);
};

#endif// FUZZER_H_