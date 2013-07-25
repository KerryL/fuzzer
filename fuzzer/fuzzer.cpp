// fuzzer.cpp

// Standard C++ headers
#include <cassert>

// MS headers
#include <process.h>
#include <tchar.h>

// Local headers
#include "fuzzer.h"

using namespace std;

Fuzzer::Fuzzer(const string &application, const unsigned int &maxExecutionTime,
		const unsigned int &maxExecutions, const unsigned int &maxBitsToFlip)
		: application(application), maxExecutionTime(maxExecutionTime),
		maxExecutions(maxExecutions), maxBitsToFlip(maxBitsToFlip)
{
	test = 0;
	crashCount = 0;

	pi.startSemaphore = CreateSemaphore(NULL, 0L, 1L, NULL);
	pi.endSemaphore = CreateSemaphore(NULL, 1L, 1L, NULL);
}

void Fuzzer::AddTestFile(const string &file)
{
	// TODO:  Check for existence of file
	files.push_back(file);
}

void Fuzzer::Fuzz(void)
{
	while (!IsFinished())
		RunNextTest();
}

void Fuzzer::RunNextTest(void)
{
	if (!SetupNextTest())
		return;// Skip this test if we failed to set it up properly

	_beginthread(LaunchTimedExecution, 0, &pi);
		
	// Wait for confirmation that the child thread has started execution
	WaitForSingleObject(pi.startSemaphore, INFINITE);

	// Wait (with timeout) for the process to end
	WaitForSingleObject(pi.endSemaphore, maxExecutionTime);

	if (!pi.done)
	{
		logBuffer << "Process timed out." << endl;
		TerminateProcess(pi.processHandle, 0);// Forces zero return value

		WaitForSingleObject(pi.endSemaphore, INFINITE);
		// Immediately release the endSemaphore - we don't want the lock,
		// it's only to signal that the thread has finished
		ReleaseSemaphore(pi.endSemaphore, 1L, NULL);
	}

	// Assume returnValue == 0 means normal exit -> not interesting, so we
	// don't log it
	if (pi.returnValue != 0)
	{
		crashCount++;
		WriteCaseToLogFile(pi.returnValue);
		cout << "Crash occured on test " << test << " for a total of "
			<< crashCount << " crashes." << endl;
	}
}

bool Fuzzer::SetupNextTest(void)
{
	test++;
	logBuffer.str(string());
	logBuffer << "Beginning test case " << test << "." << endl;
	cout << logBuffer.str();
	string fileName = ChooseAndCopyRandomInputFile();

	if (!FlipRandomBits(fileName))
	{
		logBuffer << "Fuzzer Error:  Failed to generate input file!" << endl;
		WriteCaseToLogFile(0);
		return false;
	}

	pi.command = application + " " + fileName;
	pi.returnValue = 0;
	pi.processHandle = 0;
	pi.done = false;

	return true;
}

// Selects one of the input files at random, and copies from the original
// input file to a temporary file to be be used for our test.
// Returns the name of the temporary file
string Fuzzer::ChooseAndCopyRandomInputFile(void)
{
	int selection = rand() % files.size();
	logBuffer << "Deriving test case from '"
		<< files[selection] << "'." << endl;

	string source(files[selection]);
	string destination("test_input");
	destination.append(source.substr(source.find_last_of('.')));

	ifstream in(source.c_str(), ios::binary);
	ofstream out(destination.c_str(), ios::binary);

	assert(in.is_open() && in.good());
	assert(out.is_open() && out.good());

	out << in.rdbuf();
	in.close();
	out.close();
	
	return destination;
}

// Opens the specified file and toggles a random number (range 1 to 256)
// of bits, then saves the file.
// Returns true for success, false otherwise (i.e. problem with file I/O)
bool Fuzzer::FlipRandomBits(const string &fileName)
{
	unsigned int bitsToFlip = rand() % maxBitsToFlip + 1;
	cout << "Flipping " << bitsToFlip << " bits." << endl;
	logBuffer  << "Flipping " << bitsToFlip << " bits." << endl;

	ifstream input(fileName.c_str(), ios::in | ios::binary);
	if (!input.is_open() || !input.good())
	{
		logBuffer << "!is_open() or !good() after opening file for input."
			<< endl;
		return false;
	}

	// TODO:  Remove assumption that length < MAX_INT (but for our specific
	// test files, we know it's OK)
	input.seekg(0, input.end);
	unsigned int length = (unsigned int)input.tellg();
	assert(length > 0);

	char *buffer = new char[length];
	input.seekg(0, input.beg);
	input.read(buffer, length);

	if (!input.good())
		logBuffer << "Failed to read entire contents of input file ("
		<< input.gcount() << " of " << length << " bytes read)." << endl;

	input.close();

	// Randomly flip bits
	unsigned int i;
	unsigned int bit;
	for (i = 0; i < bitsToFlip; i++)
	{
		// TODO:  Remove assumption that length < MAX_INT
		bit = rand() % length;
		logBuffer << "    Flipping bit " << bit << endl;
		FlipBit(buffer, bit);
	}

	ofstream output(fileName.c_str(), ios::out | ios::binary);
	if (!output.is_open() || !output.good())
	{
		logBuffer << "!is_open() or !good() after opening file for output."
			<< endl;
		delete [] buffer;
		return false;
	}

	output.write(buffer, length);
	output.close();

	delete [] buffer;

	return true;
}

// Flips a single bit in the buffer.
void Fuzzer::FlipBit(char *buffer, const unsigned int &bit)
{
	const unsigned bitsPerChar = 8 * sizeof(char);
	unsigned int byte = bit / bitsPerChar;
	unsigned int bitInByte = bit % bitsPerChar;
	assert(bitInByte < bitsPerChar);

	buffer[byte] ^= (1 << bitInByte);
}

// Child thread entry point.  This function launches an instance of the
// application under test and waits for it to return.
void Fuzzer::LaunchTimedExecution(void *args)
{
	ProcessInfo *pi((ProcessInfo*)args);
	while (WaitForSingleObject(pi->endSemaphore, 0L) != WAIT_OBJECT_0)
		assert(false);// Never expect above call to fail
	ReleaseSemaphore(pi->startSemaphore, 1L, NULL);

	STARTUPINFO msSi;
	PROCESS_INFORMATION msPi;
	msSi.cb = sizeof(msSi);
	msSi.lpReserved = NULL;
	msSi.lpDesktop = NULL;
	msSi.lpTitle = NULL;
	msSi.dwX = 0;
	msSi.dwY = 0;
	msSi.dwXSize = 0;
	msSi.dwYSize = 0;
	msSi.dwXCountChars = 0;
	msSi.dwYCountChars = 0;
	msSi.dwFillAttribute = 0;
	msSi.dwFlags = 0;
	msSi.cbReserved2 = 0;
	msSi.lpReserved2 = NULL;

	LPTSTR cmdLine = _tcsdup(TEXT(pi->command.c_str()));
	if (!CreateProcess(NULL, cmdLine, NULL, NULL,
		false, 0, NULL, NULL, &msSi, &msPi))
	{
		pi->processHandle = 0;
		pi->done = true;
		cout << "Failed to launch application (Error "
			<< GetLastError() << ")." << endl;
		ReleaseSemaphore(pi->endSemaphore, 1L, NULL);
		return;
	}

	pi->processHandle = msPi.hProcess;
	pi->returnValue = WaitForSingleObject(pi->processHandle, INFINITE);
	pi->done = true;

	ReleaseSemaphore(pi->endSemaphore, 1L, NULL);
}

// Appends information about a test case to the log file.
void Fuzzer::WriteCaseToLogFile(const int &returnValue)
{
	ofstream log("fuzzer.log", ios::app);
	if (!log.is_open() || !log.good())
	{
		cout << "Failed to open log file!" << endl;
		cout << "Attempted to write:" << endl;
		cout << logBuffer << endl;
		return;
	}

	logBuffer << "Return code (case " << test << "): "
		<< returnValue << endl;

	// Also print to the screen
	cout << logBuffer.str() << endl << endl;
	log << logBuffer.str() << endl << endl;

	log.close();
}