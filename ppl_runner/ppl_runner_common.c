/*
Common Code and helper functions for ppl_runner
*/
#include <stdio.h>
#include <Windows.h>
#include "ppl_runner.h"

VOID log_message(WCHAR* format, ...)
{
    WCHAR message[MAX_BUF_SIZE];

    va_list arg_ptr;
    va_start(arg_ptr, format);
    int ret = _vsnwprintf_s(message, MAX_BUF_SIZE, MAX_BUF_SIZE, format, arg_ptr);
    va_end(arg_ptr);
    OutputDebugString(message);
    wprintf(message);
}
