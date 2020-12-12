/*
ppl_runner
*/

#include <stdio.h>
#include <Windows.h>
#include "ppl_runner.h"


DWORD install_elam_cert()
{
    DWORD retval = 0;
    HANDLE fileHandle = NULL;
    WCHAR driverName[] = DRIVER_NAME;

    log_message(L"[PPL_RUNNER] install_elam_cert: Opening driver file: %s\n", driverName);
    fileHandle = CreateFile(driverName,
        FILE_READ_DATA,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (fileHandle == INVALID_HANDLE_VALUE) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] install_elam_cert: CreateFile Error: %d\n", retval);
        return retval;
    }

    if (InstallELAMCertificateInfo(fileHandle) == FALSE) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] install_elam_cert: install_elam_certificateInfo Error: %d\n", retval);
        return retval;
    }
    log_message(L"[PPL_RUNNER] install_elam_cert: Installed ELAM driver cert\n");

    return retval;
}

DWORD install_service()
{
    DWORD retval = 0;
    SERVICE_LAUNCH_PROTECTED_INFO info;
    SC_HANDLE hService;
    SC_HANDLE hSCManager;

    DWORD SCManagerAccess = SC_MANAGER_ALL_ACCESS;
    hSCManager = OpenSCManager(NULL, NULL, SCManagerAccess);
    if (hSCManager == NULL) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] install_service: OpenSCManager Error: %d\n", retval);
        return retval;

    }

    // Get full path to ourselves with 'service' argv
    WCHAR serviceCMD[MAX_BUF_SIZE] = { 0 };
    GetModuleFileName(NULL, serviceCMD, MAX_BUF_SIZE);
    DWORD serviceCMDLen = lstrlenW(serviceCMD);
    wcscpy_s(serviceCMD + serviceCMDLen, MAX_BUF_SIZE - serviceCMDLen, L" service");

    // Add PPL option
    info.dwLaunchProtected = SERVICE_LAUNCH_PROTECTED_ANTIMALWARE_LIGHT;
    hService = CreateService(
        hSCManager,
        SERVICE_NAME,
        SERVICE_NAME,
        SCManagerAccess,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        serviceCMD,
        NULL,
        NULL,
        NULL,
        NULL, /* ServiceAccount */
        NULL
    );

    if (hService == NULL) {
        retval = GetLastError();
        if (retval == ERROR_SERVICE_EXISTS) {
            log_message(L"[PPL_RUNNER] install_service: CreateService Error: Service '%s' Already Exists\n", SERVICE_NAME);
            log_message(L"[PPL_RUNNER] install_service: Run 'net start %s' to start the service\n", SERVICE_NAME);
        }
        else {
            log_message(L"[PPL_RUNNER] install_service: CreateService Error: %d\n", retval);
        }
        return retval;
    }

    // Mark service as protected
    if (ChangeServiceConfig2(hService, SERVICE_CONFIG_LAUNCH_PROTECTED, &info) == FALSE) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] install_service: ChangeServiceConfig2 Error: %d\n", retval);
        return retval;
    }
    log_message(L"[PPL_RUNNER] install_service: install_service: Created Service: %s\n", serviceCMD);
    log_message(L"[PPL_RUNNER] install_service: Run 'net start %s' to start the service\n", SERVICE_NAME);
    return retval;
}


DWORD remove_service() {
    DWORD retval = 0;
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    SERVICE_STATUS_PROCESS ssp;
    DWORD dwBytesNeeded;
    log_message(L"[PPL_RUNNER] remove_service: Stopping and Deleting Service %s...\n", SERVICE_NAME);

    // Get Handle to Service Manager and Service
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] remove_service: OpenSCManager Error: %d\n", retval);
        return retval;

    }
    hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (hService == NULL) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] remove_service: OpenService Error: %d\n", retval);
        return retval;
    }

    // Get status of service
    if (!QueryServiceStatusEx(
        hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] remove_service: QueryServiceStatusEx1 Error: %d\n", retval);
        return retval;
    }

    if (ssp.dwCurrentState != SERVICE_STOPPED) {
        // Send a stop code to the service.
        if (!ControlService(hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp)) {
            retval = GetLastError();
            log_message(L"[PPL_RUNNER] remove_service: ControlService(Stop) Error: %d\n", retval);
            return retval;
        }
        if (ssp.dwCurrentState != SERVICE_STOPPED) {
            // Wait for service to die
            Sleep(3000);
            if (!QueryServiceStatusEx(
                hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
                retval = GetLastError();
                log_message(L"[PPL_RUNNER] remove_service: QueryServiceStatusEx2 Error: %d\n", retval);
                return retval;
            }
            if (ssp.dwCurrentState != SERVICE_STOPPED) {
                retval = ssp.dwCurrentState;
                log_message(L"[PPL_RUNNER] remove_service: Waited but service stull not stopped: %d\n", retval);
                return retval;
            }
        }
    }

    // Service stopped, now remove it
    if (!DeleteService(hService)) {
        retval = GetLastError();
        log_message(L"[PPL_RUNNER] remove_service: DeleteService Error: %d\n", retval);
        return retval;
    }

    log_message(L"[PPL_RUNNER] remove_service: Deleted Service %s\n", SERVICE_NAME);

    return retval;
}


DWORD main(INT argc, CHAR** argv)
{
    DWORD retval = 0;
    log_message(L"[PPL_RUNNER] main: Start\n");

    if (argc != 2) {
        log_message(L"[PPL_RUNNER] usage: ppl_runner.exe <install|service|remove>\n");
        retval = 1;
    }
    else if (strcmp(argv[1], "install") == 0) {
        log_message(L"[PPL_RUNNER] setting up ELAM stuff...\n");
        retval = install_elam_cert();
        if (retval == 0) {
            log_message(L"[PPL_RUNNER] Installing Service...\n");
            retval = install_service();
        }
    }
    else if (strcmp(argv[1], "service") == 0) {
        log_message(L"[PPL_RUNNER] Starting as a service...\n");
        retval = service_entry();
    }
    else if (strcmp(argv[1], "remove") == 0) {
        log_message(L"[PPL_RUNNER] Removing Service...\n");
        retval = remove_service();
    }
    else {
        log_message(L"[PPL_RUNNER] invalid commandline option\n");
        retval = 1;
    }

    return retval;
}
