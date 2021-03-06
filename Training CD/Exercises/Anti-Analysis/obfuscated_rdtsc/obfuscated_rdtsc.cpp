#include "stdafx.h"
#include <windows.h>

/*
    WINXPSP2.NTDLL.ReleaseBufferLocation():
        .text:7C973215     mov ecx, [eax+4]                     // we *could* jump as high as this point
        .text:7C973218     add ecx, eax
        .text:7C97321A     mov dword ptr [ecx], 0C0100001h
   ---> .text:7C973220     rdtsc                                // ecx should point to the caller stack
        .text:7C973222     mov [ecx+8], eax                     // save the low dword
        .text:7C973225     mov [ecx+0Ch], edx                   // save the high dword
        .text:7C973228
        .text:7C973228 loc_7C973228:
        .text:7C973228     add esi, 0Ch
        .text:7C97322B     or  eax, 0FFFFFFFFh
        .text:7C97322E     lock xadd [esi], eax                 // esi must point to writeable memory
        .text:7C973232     mov eax, offset _WmipLoggerCount
        .text:7C973237     or  ecx, 0FFFFFFFFh
        .text:7C97323A     lock xadd [eax], ecx
        .text:7C97323E     pop esi                              // esi gets blows away, save/restore if needed
        .text:7C97323F     pop ebp                              // ebp gets blown away, save/restore in caller
        .text:7C973240     retn 4                               // need to pad two DWORDS for RET address
*/

int obfuscated_rdtsc (void)
{
    int stack_space[32];
    
    __asm
    {
            jmp __end
        __begin:
            pop ebx
            mov ecx, esp
            add ecx, 4
            mov esi, esp
            add esi, 0x10
            push ebp
            push 1
            push ebx
            push 2
            push 3
            mov eax, 0x7c973220
            jmp eax
        __end:
            call __begin
            pop ebp
    }

    //printf("%08x:%08x\n", stack_space[1], stack_space[0]);
	return stack_space[0];
}

int main(int argc, char* argv[])
{
    int start;
    int end;
    int threshold = 0x6000;

    start = obfuscated_rdtsc();
    end   = obfuscated_rdtsc();
    
    // observed values range from high 0x3000's to mid 0x4000's on my personal laptop.
    printf("%08x\n", end - start);

    if (end - start > threshold)
        MessageBox(0, "Debugger detected!", "Debugger detected!", 0);

    return 0;
}