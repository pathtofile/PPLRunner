/*
ppl_runner Service
Code for the Service that gets run
*/
#include <stdio.h>
#include <Windows.h>
#include "ppl_runner.h"

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;

DWORD start_child_process()
{
    DWORD retval = 0;
    WCHAR childCMD[MAX_BUF_SIZE] = { 0 };
    DWORD dataSize = MAX_BUF_SIZE;
    log_message(L"[PPL_RUNNER] start_child_process: Starting");

    // Get Command to run from registry
    log_message(L"[PPL_RUNNER] start_child_process: Looking for command in RegKey: HKLM\\%s\n", CMD_REGKEY);

    retval = RegGetValue(HKEY_LOCAL_MACHINE, CMD_REGKEY, NULL, RRF_RT_REG_SZ, NULL, &childCMD, &dataSize);
    if (retval != ERROR_SUCCESS) {
        log_message(L"[PPL_RUNNER] start_child_process: RegGetValue Error: %d\n", retval);
        return retval;
    }

    // Create Attribute List
    STARTUPINFOEXW StartupInfoEx = { 0 };
    SIZE_T AttributeListSize = 0;
    StartupInfoEx.StartupInfo.cb = sizeof(StartupInfoEx);
    InitializeProcThreadAttributeList(NULL, 1, 0, &AttributeListSize);
    if (AttributeListSize == 0) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] start_child_process: InitializeProcThreadAttributeList1 Error: %d\n", retval);
        return retval;
    }
    StartupInfoEx.lpAttributeList =
        (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, AttributeListSize);
    if (InitializeProcThreadAttributeList(StartupInfoEx.lpAttributeList, 1, 0, &AttributeListSize) == FALSE) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] start_child_process: InitializeProcThreadAttributeList2 Error: %d\n", retval);
        return retval;
    }

    // Set ProtectionLevel to be the same, i.e. PPL
    DWORD ProtectionLevel = PROTECTION_LEVEL_SAME;
    if (UpdateProcThreadAttribute(StartupInfoEx.lpAttributeList,
        0,
        PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL,
        &ProtectionLevel,
        sizeof(ProtectionLevel),
        NULL,
        NULL) == FALSE)
    {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] start_child_process: UpdateProcThreadAttribute Error: %d\n", retval);
        return retval;
    }

    // Start Process (hopefully)
    PROCESS_INFORMATION ProcessInformation = { 0 };
    log_message(L"[PPL_RUNNER] start_child_process: Creating Process: '%s'\n", childCMD);
    if (CreateProcess(NULL,
        childCMD,
        NULL,
        NULL,
        FALSE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_PROTECTED_PROCESS,
        NULL,
        NULL,
        (LPSTARTUPINFOW)&StartupInfoEx,
        &ProcessInformation) == FALSE)
    {
        retval = GetLastError();
        if (retval == ERROR_INVALID_IMAGE_HASH) {
            log_message(L"[PPL_RUNNER] start_child_process: CreateProcess Error: Invalid Certificate\n");
        }
        else {
            log_message(L"[PPL_RUNNER] start_child_process: CreateProcess Error: %d\n", retval);
        }
        return retval;
    }
    // Don't wait on process handle, we're setting our child free into the wild
    // This is to prevent any possible deadlocks

    log_message(L"[PPL_RUNNER] start_child_process finished");
    return retval;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD retval = 0;
    log_message(L"[PPL_RUNNER] ServiceMain: Starting\n");

    // Register our service control handler with the SCM
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, service_ctrl_handler);
    if (g_StatusHandle == NULL)
    {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] ServiceMain: Registerservice_ctrl_handler Error: %d\n", retval);
        return;
    }
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    // Start Child Process
    retval = start_child_process();

    // Tell the service controller we are stopped
    // So we can be run again
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = retval;
    g_ServiceStatus.dwServiceSpecificExitCode = retval;
    g_ServiceStatus.dwCheckPoint = 0;
    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] ServiceMain: SetServiceStatus(Stopped) Error: %d\n", retval);
        return;
    }

    log_message(L"[PPL_RUNNER] ServiceMain: Finished");
    return;
}


VOID WINAPI service_ctrl_handler(DWORD ctrlCode)
{
    // It should be very unlikley for this function to be called,
    // as our service only lives long enough to launch the child process
    DWORD retval = 0;

    log_message(L"[PPL_RUNNER] Starting service_ctrl_handler");
    switch (ctrlCode)
    {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;
        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            retval = GetLastError();
            log_message(L"[PPL_RUNNER] ServiceMain: SetServiceStatus(StopPending) Error: %d\n", retval);
            return;
        }
        break;

    default:
        break;
    }
}


DWORD service_entry()
{
    // Basic boilerplate service setup
    DWORD retval = 0;
    SERVICE_TABLE_ENTRY serviceTable[] =
    {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(serviceTable) == FALSE)
    {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] service_entry: StartServiceCtrlDispatcher error: %d\n", retval);
        return retval;
    }

    return retval;
}
