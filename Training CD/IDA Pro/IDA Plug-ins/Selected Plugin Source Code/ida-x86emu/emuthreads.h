/*
   Source for x86 emulator IdaPro plugin
   File: emuthreads.h
   Copyright (c) 2006, Chris Eagle
   
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option) 
   any later version.
   
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
   more details.
   
   You should have received a copy of the GNU General Public License along with 
   this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
   Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef __EMU_THREADS_H
#define __EMU_THREADS_H

#include "cpu.h"
#include "buffer.h"

#define THREAD_MAGIC 0xDEADBEEF
#define THREAD_ID_BASE 0x500
#define THREAD_HANDLE_BASE 0x700

class MemoryManager;
class EmuStack;

class ThreadNode {
public:
   ThreadNode(EmuStack *stack);
   ThreadNode(dword threadFunc, dword threadArg);
   ThreadNode(Buffer &b, dword currentActive);
   
   void save(Buffer &b, bool saveStack);

   dword handle;
   dword id;
   dword hasStarted;
   dword threadArg;
   Registers regs;
   EmuStack *stack;
   ThreadNode *next;
};

extern ThreadNode *threadList;
extern ThreadNode *activeThread;

/*
 * return thread handle for new thread
 */
ThreadNode *emu_create_thread(dword threadFunc, dword threadArg);

/*
 * destroy the thread indicated by threadId
 */
ThreadNode *emu_destroy_thread(dword threadId);

/*
 * switch threads
 */
void emu_switch_threads(MemoryManager *mgr, ThreadNode *new_thread);

/*
 * locate the thread with the given handle
 */
ThreadNode *findThread(dword handle);

/*
 * See if some threads stack contains the specified address
 */
ThreadNode *threadStackContains(dword addr);

#endif
