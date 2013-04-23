// OpenOrSwitchToNative.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <Psapi.h>
#include <string>
#include <strsafe.h>
#include <windowsx.h>

const DWORD MaxProcessIds = 1024;
const DWORD MaxExeNameLength = 2048;

DWORD targetProcessId = 0;
HWND targetWindowHandle = 0;

void PrintError() 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    _tprintf(
        TEXT("failed with error %d: %s"), 
        dw, lpMsgBuf);

    LocalFree(lpMsgBuf);
}

#define LOG(operation) { DWORD result = operation; _tprintf(_T("%s: %d\n"), _T(#operation), result); if (!result) PrintError(); }

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == targetProcessId && GetWindow(hwnd, GW_OWNER) && IsWindowVisible(hwnd))
    {
        targetWindowHandle = hwnd;
        //return FALSE;
    }
    return TRUE;
}

int APIENTRY _tWinMain(
    HINSTANCE handleToInstance,
    HINSTANCE handleToPreviousInstance,
    LPTSTR    commandLine,
    int       showCommand)
{
    LPTSTR commandLineReader = commandLine;

    int argc = 0;
    while (*commandLineReader)
    {
        if (*commandLineReader == _T(' ')) argc++;
        commandLineReader++;
    }

    /*if (argc < 2)
    {
        return 0;
    }*/
    
    TCHAR targetExecutableBuffer[MaxExeNameLength];
    _tcscpy_s<MaxExeNameLength>(targetExecutableBuffer, commandLine);
    LPTSTR targetExecutable = targetExecutableBuffer;

    if (*targetExecutable == TEXT('"'))
    {
        targetExecutable++;
        *_tcschr(targetExecutable, TEXT('"')) = TEXT('\0');
    }

    // See if the app is already running; if so, activate it
    
    // Get all running process ids

    DWORD processIds[MaxProcessIds];
    DWORD bytesNeeded;

    EnumProcesses(processIds, MaxProcessIds * sizeof(DWORD), &bytesNeeded);
    DWORD processCount = bytesNeeded / sizeof(DWORD);
    
    for (DWORD i = 0; i < processCount; i++)
    {
        // Get the image name of the process

        TCHAR exeName[MaxExeNameLength];
        DWORD length = MaxExeNameLength * sizeof(TCHAR);
        HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processIds[i]);
        BOOL result = QueryFullProcessImageName(processHandle, 0, exeName, &length);
        CloseHandle(processHandle);

        // If the image name is the same as the one we are trying to start

        if (result && _tcsicmp(targetExecutable, exeName) == 0)
        {
            // Get its window handle

            targetProcessId = processIds[i];
            EnumWindows(EnumWindowsProc, 0);

            // Restore the window if it is minimized
            BOOL isMinimized = !IsMaximized(targetWindowHandle);
            if (isMinimized)
            {
                ShowWindow(targetWindowHandle, SW_RESTORE);
            }

            // Bring the window to the front (activate it)

            DWORD currentThreadId = GetCurrentThreadId();
            DWORD targetThreadId = GetWindowThreadProcessId(targetWindowHandle, NULL);

            AttachThreadInput(currentThreadId, targetThreadId, TRUE);

            SetActiveWindow(targetWindowHandle);

            AttachThreadInput(currentThreadId, targetThreadId, FALSE);

            return 0;
        }
    }

    // App wasn't found, so start it

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    CreateProcess(
        NULL,
        commandLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi);

    return 0;
}

