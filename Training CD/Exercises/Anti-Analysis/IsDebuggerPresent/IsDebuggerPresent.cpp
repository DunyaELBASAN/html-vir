// IsDebuggerPresent.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

typedef BOOL (WINAPI *lpfIsDebuggerPresent) (VOID);
lpfIsDebuggerPresent pIsDebuggerPresent = NULL;


int main(int argc, char* argv[])
{
    HMODULE h_kernel32;

    if ((h_kernel32 = LoadLibrary("kernel32.dll")) == NULL)
    {
        printf("Failed loading library kernel32.dll.\n");
        return 1;
    }

    pIsDebuggerPresent = (lpfIsDebuggerPresent)  GetProcAddress(h_kernel32, "IsDebuggerPresent");
    
    if (!pIsDebuggerPresent)
    {
        printf("Failed to resolve kernel32.IsDebuggerPresent().\n");
        return 1;
    }


	if (pIsDebuggerPresent())
    {
        MessageBox(0, "Debugger detected", "Debugger detected", 0);
        printf("Debugger detected.\n");
    }
    else
    {
        MessageBox(0, "No debugger detected, executing normally.", "No debugger detected", 0);
        printf("No debugger detected, executing normally.\n");
    }

	return 0;
}

