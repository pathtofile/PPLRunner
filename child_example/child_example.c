/*
This is an example Process that can be run by the ppl_runner service

Visual Studio will build and sing the binary, so it can be run the the service as PPL
*/

#include <Windows.h>
#include <stdio.h>

void log_msg(WCHAR* format, ...)
{
    WCHAR log_msg[1024];

    va_list arg_ptr;
    va_start(arg_ptr, format);
    int ret = _vsnwprintf_s(log_msg, 1024, 1024, format, arg_ptr);
    va_end(arg_ptr);
    OutputDebugString(log_msg);
    wprintf(log_msg);
}

DWORD main(int argc, char** argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    for (DWORD i = 0; i < 100; i++) {
        log_msg(L"[PPL_RUNNER] Inside Child Runner: %02d\n", i);
        Sleep(3000);
    }
    return 0;
}
