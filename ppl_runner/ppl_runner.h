#pragma once

// These are the globals params that can be changed
#define SERVICE_NAME  L"ppl_runner"
#define DRIVER_NAME L"elam_driver.sys"
#define CMD_REGKEY L"SOFTWARE\\PPL_RUNNER"
#define MAX_BUF_SIZE 1024


VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI service_ctrl_handler(DWORD);
DWORD service_entry();

VOID log_message(WCHAR* format, ...);
