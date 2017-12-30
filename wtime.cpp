#include <Windows.h>
#include <stdio.h>

#ifndef __GNUC__
#define MAIN(x,y) wmain(int x,wchar_t ** y)
#endif

#ifdef __GNUC__
#define MAIN(x,y) main()
#ifdef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION PROCESS_QUERY_INFORMATION
#else
#define PROCESS_QUERY_LIMITED_INFORMATION (0x0400)
#endif
#define fwprintf_s fwprintf
#define wprintf_s wprintf
#endif
#define PER_SECOND (1000.0*1000*10)

const wchar_t help[] = L"+-------+\n"\
						"| WTIME |\n"\
						"+-------+\n"\
						"Usage: wtime [-ifilename -ofilename] <cmd> [arg1] [arg2] ...\n\n";

int MAIN(argc,argv)
{
#ifdef __GNUC__
	int argc;
	wchar_t ** argv = CommandLineToArgvW(GetCommandLineW(),&argc);
#endif
	if (argc <= 1)
	{
		wprintf_s(help);
		_wsystem(L"pause");
		return EXIT_FAILURE;
	}

	wchar_t * in = nullptr, * out = nullptr;
	int cur = 0;

	for (int i = 1;i < ((argc>=3)?3:argc);++i)
		if (argv[i][0] == '-')
		{
			if (argv[i][1] == 'i') { in = argv[i] + 2; cur = cur < i ? i : cur; }
			else if (argv[i][1] == 'o') { out = argv[i] + 2; cur = cur < i ? i : cur; }
		} 
	if ((in && !out) || (out && !in)) { fwprintf_s(stderr, L"Lack of '-ifile' or '-ofile'.\n"); return EXIT_FAILURE; }

	HANDLE hOut = INVALID_HANDLE_VALUE, hIn = INVALID_HANDLE_VALUE;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE };
	
	if (out) hOut = CreateFileW(out,
		GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		&sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (in) hIn = CreateFileW(in,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		&sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (out&&hOut==INVALID_HANDLE_VALUE) 
	{
		fwprintf_s(stderr, L"Redirect failed.\n"); return EXIT_FAILURE;
	}
	if (in&&hIn == INVALID_HANDLE_VALUE)
	{
		fwprintf_s(stderr, L"Redirect failed.\n"); return EXIT_FAILURE;
	}

	int len = 0;

	for (int i = cur+1;i < argc;++i)
		len += wcslen(argv[i])+1;
	wchar_t * p = argv[cur + 1];
	for (int i = 0; i < len; ++i, ++p)
		if (*p == '\0') *p = ' ';
	*(p - 1) = '\0';

	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	si.cb = sizeof(STARTUPINFOW);

	if (hIn != INVALID_HANDLE_VALUE && hOut != INVALID_HANDLE_VALUE)
	{
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = hIn;
		si.hStdError = hOut;
		si.hStdOutput = hOut;
	}
	
	if (!CreateProcessW(nullptr, argv[cur+1], nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi))
	{
		fwprintf_s(stderr,L"CreateProcess (%ws) failed (%d).\n",argv[cur+1], GetLastError()); return EXIT_FAILURE;
	}

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pi.dwProcessId);

	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	

	FILETIME createTime = { 0 }, exitTime = { 0 }, kernelTime = { 0 }, userTime = { 0 };
	if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime))
	{
		fwprintf_s(stderr,L"GetProcessTimes failed (%d).\n", GetLastError()); 
		CloseHandle(hProcess);
		return EXIT_FAILURE;
	}

	CloseHandle(hProcess);

	ULARGE_INTEGER t1,t2;

	t1.LowPart = createTime.dwLowDateTime;
	t1.HighPart = createTime.dwHighDateTime;
	t2.LowPart = exitTime.dwLowDateTime;
	t2.HighPart = exitTime.dwHighDateTime;

	fwprintf_s(stderr, L"\nreal:\t%0.5lfs\n", (double)(t2.QuadPart - t1.QuadPart) / PER_SECOND);

	t1.LowPart = userTime.dwLowDateTime;
	t1.HighPart = userTime.dwHighDateTime;
	t2.LowPart = kernelTime.dwLowDateTime;
	t2.HighPart = kernelTime.dwHighDateTime;

	fwprintf_s(stderr, L"user:\t%0.5lfs\n", (double)(t1.QuadPart) / PER_SECOND);
	fwprintf_s(stderr, L"kernel:\t%0.5lfs\n", (double)(t2.QuadPart) / PER_SECOND);

	return EXIT_SUCCESS;
}
