/*
   Source for x86 emulator IdaPro plugin
   File: emufuncs.cpp
   Copyright (c) 2004, Chris Eagle
   
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

#include <windows.h>
#include <winnt.h>

#ifndef USE_STANDARD_FILE_FUNCTIONS
#define USE_STANDARD_FILE_FUNCTIONS 1
#endif

#ifdef CYGWIN
#include <psapi.h>
#endif

#include "cpu.h"
#include "emufuncs.h"
#include "memmgr.h"
#include "hooklist.h"
#include "emuthreads.h"
#include "buffer.h"
#include "peutils.h"

#include <kernwin.hpp>
#include <bytes.hpp>
#include <name.hpp>

#define FAKE_HANDLE_BASE 0x80000000

extern HWND x86Dlg;

struct HandleList {
   char *moduleName;
   dword handle;
   dword id;
   dword maxAddr;
   dword ordinal_base;
   dword NoF;  //NumberOfFunctions
   dword NoN;  //NumberOfNames
   dword eat;  //AddressOfFunctions  export address table
   dword ent;  //AddressOfNames      export name table
   dword eot;  //AddressOfNameOrdinals  export ordinal table
   HandleList *next;
};

struct FakedImport {
   dword handle;  //module handle the lookup was performed on
   dword addr;    //returned fake import address
   char *name;    //name assigned to this function
   FakedImport *next;
};

static HandleList *moduleHead = NULL;
static FunctionInfo *functionInfoList = NULL;

static FakedImport *fakedImportList = NULL;
static dword fakedImportAddr = 1;

//stick dummy values up in kernel space to distinguish them from
//actual library handles
static dword moduleHandle = FAKE_HANDLE_BASE;    

//persistant module identifier
static dword moduleId = 0x10000;

static unemulatedCB unemulated_cb = NULL;

typedef enum {R_FAKE = -1, R_NO = 0, R_YES = 1} Reply;

int emu_alwaysLoadLibrary = ASK;
int emu_alwaysGetModuleHandle = ASK;

HookEntry hookTable[] = {
   {"CreateThread", emu_CreateThread},
   {"VirtualAlloc", emu_VirtualAlloc},
   {"VirtualFree", emu_VirtualFree},
   {"LocalAlloc", emu_LocalAlloc},
   {"LocalFree", emu_LocalFree},
   {"GetProcAddress", emu_GetProcAddress},
   {"GetModuleHandleA", emu_GetModuleHandle},
   {"LoadLibraryA", emu_LoadLibrary},
   {"HeapCreate", emu_HeapCreate},
   {"HeapDestroy", emu_HeapDestroy},
   {"HeapAlloc", emu_HeapAlloc},
   {"RtlAllocateHeap", emu_HeapAlloc},
   {"HeapFree", emu_HeapFree},
   {"GetProcessHeap", emu_GetProcessHeap},
   {"malloc", emu_malloc},
   {"calloc", emu_calloc},
   {"realloc", emu_realloc},
   {"free", emu_free},
   {NULL, NULL}
};

HandleList *findModuleByName(char *h) {
   HandleList *hl;
   for (hl = moduleHead; hl; hl = hl->next) {
      if (stricmp(h, hl->moduleName) == 0) break;
   }
   return hl;
}

HandleList *findModuleByHandle(dword handle) {
   HandleList *hl;
   for (hl = moduleHead; hl; hl = hl->next) {
      if (hl->handle == handle) break;
      if (hl->id == handle) break;       //for compatibility with old handle assignment style
   }
   return hl;
}

unsigned int getPEoffset(HMODULE mod) {
   IMAGE_DOS_HEADER *hdr = (IMAGE_DOS_HEADER*) mod;
   if (mod >= (HMODULE)FAKE_HANDLE_BASE) return 0;
   if (hdr->e_magic == IMAGE_DOS_SIGNATURE) {
      return hdr->e_lfanew;
   }
   return 0;
}

IMAGE_NT_HEADERS *getPEHeader(HMODULE mod) {
   unsigned int offset = getPEoffset(mod);
   if (offset == 0) return NULL;
   IMAGE_NT_HEADERS *pe = (IMAGE_NT_HEADERS *)(offset + (char*)mod);
   if (pe->Signature != IMAGE_NT_SIGNATURE) {
      pe = NULL;
   }
   return pe;
}

//find an existing faked import
FakedImport *findFakedImportByAddr(HandleList *mod, dword addr) {
   FakedImport *ff = NULL;
   for (ff = fakedImportList; ff; ff = ff->next) {
      if (ff->handle == mod->handle && ff->addr == addr) break;
   }
   return ff;
}

FakedImport *findFakedImportByName(HandleList *mod, char *procName) {
   FakedImport *ff = NULL;
   for (ff = fakedImportList; ff; ff = ff->next) {
      if (ff->handle == mod->handle && strcmp(ff->name, procName) == 0) break;
   }
   return ff;
}

//add a new faked import after first searching to see if it is already present
FakedImport *addFakedImport(HandleList *mod, char *procName) {
   FakedImport *ff = findFakedImportByName(mod, procName);
   if (ff) return ff;
   ff = (FakedImport*)malloc(sizeof(FakedImport));
   ff->next = fakedImportList;
   ff->addr = mod->maxAddr++;
   ff->name = strdup(procName);
   ff->handle = mod->handle;
   fakedImportList = ff;
   return ff;
}

HandleList *addModule(char *mod, int id) {
   msg("x86emu: addModule called for %s\n", mod);
   HandleList *m = findModuleByName(mod);
   if (m == NULL) {
      dword h = 0;
      dword len;

      char module_name[260];
      if ((id & FAKE_HANDLE_BASE) != 0) {
         h = id;
      }
      if (h == 0) {
         len = GetSystemDirectory(module_name, sizeof(module_name));
         module_name[len++] = '\\';
         module_name[len] = 0;
         strncat(module_name, mod, sizeof(module_name) - len - 1);
         module_name[sizeof(module_name) - 1] = 0;
         FILE *f = fopen(module_name, "rb");
         if (f == NULL) {
            int load = R_YES;
            load = askbuttons_c("Yes", "No", "Fake it", 1, "Could not locate %s. Locate it now?", mod);
            if (load == R_YES) {
               char *fname = askfile_c(false, mod, "Please locate dll %s", mod);
               if (fname) {
                  f = fopen(fname, "rb");
               }
            }
         }
         if (f) {
            h = loadIntoIdb(f);
            if (h == 0xFFFFFFFF) h = 0;
            fclose(f);
         }
         if (h == 0) {
            warning("Failure loading %s, faking it.", mod);
            h = FAKE_HANDLE_BASE + moduleId;
            moduleId += 0x10000;
         }
      }
      m = (HandleList*) calloc(1, sizeof(HandleList));
      m->next = moduleHead;
      moduleHead = m;
      m->moduleName = strdup(mod);
      m->handle = (dword) h;
      if (h & FAKE_HANDLE_BASE) {
         //faked module with no loaded export table
         m->maxAddr = h + 1;
      }
      else {  //good module load
         m->id = id ? id : moduleId;
         moduleId += 0x10000;
   
         dword pe_addr = m->handle + get_long(0x3C + m->handle);  //dos.e_lfanew
         m->maxAddr = m->handle + get_long(pe_addr + 0x18 + 0x38); //nt.OptionalHeader.SizeOfImage
   
         dword export_dir = m->handle + get_long(pe_addr + 0x18 + 0x60 + 0x0);  //export dir RVA
    
         m->ordinal_base = get_long(export_dir + 0x10);   //ed.Base
   
         m->NoF = get_long(export_dir + 0x14);  //ed.NumberOfFunctions
         m->NoN = get_long(export_dir + 0x18);  //ed.NumberOfNames
   
         m->eat = m->handle + get_long(export_dir + 0x1C);  //ed.AddressOfFunctions;
         m->ent = m->handle + get_long(export_dir + 0x20);  //ed.AddressOfNames;
         m->eot = m->handle + get_long(export_dir + 0x24);  //ed.AddressOfNameOrdinals;
      }
   }
   return m;
}

void freeModuleList() {
   for (HandleList *p = moduleHead; p; moduleHead = p) {
      p = p->next;
      free(moduleHead->moduleName);
      free(moduleHead);
   }
   for (FakedImport *f = fakedImportList; f; fakedImportList = f) {
      f = f->next;
      free(fakedImportList->name);
      free(fakedImportList);
   }
   fakedImportList = NULL;
   moduleHead = NULL;
   moduleHandle = FAKE_HANDLE_BASE;
}

void loadModuleList(Buffer &b) {
//   freeModuleList();
   int n;
   b.read((char*)&n, sizeof(n));
   for (int i = 0; i < n; i++) {
      dword id, tempid;
      char *name;
      b.read((char*)&id, sizeof(id));
      tempid = id & ~FAKE_HANDLE_BASE;
      if (tempid > moduleId) moduleId = tempid + 1;
      b.readString(&name);
      if (findModuleByName(name) == NULL) {
         HandleList *m = addModule(name, id);
         m->next = moduleHead;
         moduleHead = m;
      }
      free(name);
   }
}

void saveModuleList(Buffer &b) {
   int n = 0;
   for (HandleList *p = moduleHead; p; p = p->next) n++;
   b.write((char*)&n, sizeof(n));
   for (HandleList *m = moduleHead; m; m = m->next) {
      dword moduleId = m->id | (m->handle & FAKE_HANDLE_BASE); //set high bit of id if using fake handle
      b.write((char*)&moduleId, sizeof(moduleId));
      b.writeString(m->moduleName);
   }
}

void loadModuleData(Buffer &b) {
   freeModuleList();
   int n = 0;
   b.read((char*)&n, sizeof(n));
   for (int i = 0; i < n; i++) {
      HandleList *m = (HandleList*)malloc(sizeof(HandleList));
      m->next = moduleHead;
      moduleHead = m;

      b.read((char*)&m->handle, sizeof(m->handle));
      b.read((char*)&m->id, sizeof(m->id));
      b.read((char*)&m->maxAddr, sizeof(m->maxAddr));
      b.read((char*)&m->ordinal_base, sizeof(m->ordinal_base));
      b.read((char*)&m->NoF, sizeof(m->NoF));
      b.read((char*)&m->NoN, sizeof(m->NoN));
      b.read((char*)&m->eat, sizeof(m->eat));
      b.read((char*)&m->ent, sizeof(m->ent));
      b.read((char*)&m->eot, sizeof(m->eot));
      b.readString(&m->moduleName);
      
      if (m->id > moduleId) {
         moduleId = m->id + 0x10000;
      }
   }
   b.read((char*)&n, sizeof(n));
   for (int j = 0; j < n; j++) {
      FakedImport *f = (FakedImport*)malloc(sizeof(FakedImport));
      b.read(&f->handle, sizeof(f->handle));  //module handle the lookup was performed on
      b.read(&f->addr, sizeof(f->addr));    //returned fake import address
      b.readString(&f->name);    //name assigned to this function
   }
}

void saveModuleData(Buffer &b) {
   int n = 0;
   for (HandleList *p = moduleHead; p; p = p->next) n++;
   b.write((char*)&n, sizeof(n));
   for (HandleList *m = moduleHead; m; m = m->next) {
      b.write((char*)&m->handle, sizeof(m->handle));
      b.write((char*)&m->id, sizeof(m->id));
      b.write((char*)&m->maxAddr, sizeof(m->maxAddr));
      b.write((char*)&m->ordinal_base, sizeof(m->ordinal_base));
      b.write((char*)&m->NoF, sizeof(m->NoF));
      b.write((char*)&m->NoN, sizeof(m->NoN));
      b.write((char*)&m->eat, sizeof(m->eat));
      b.write((char*)&m->ent, sizeof(m->ent));
      b.write((char*)&m->eot, sizeof(m->eot));
      b.writeString(m->moduleName);
   }
   //now save our FakedImport list as well
   n = 0;
   for (FakedImport *f = fakedImportList; f; f = f->next) n++;
   b.write((char*)&n, sizeof(n));
   for (FakedImport *i = fakedImportList; i; i = i->next) {
      b.write(&i->handle, sizeof(i->handle));  //module handle the lookup was performed on
      b.write(&i->addr, sizeof(i->addr));    //returned fake import address
      b.writeString(i->name);    //name assigned to this function
   }
}

/*
 * Build a C string by reading from the specified address until a NULL is
 * encountered.  Returned value must be free'd
 *
 */

char *getString(MemoryManager *mgr, dword addr) {
   int size = 16;
   int i = 0;
   byte *str = NULL, ch;
   str = (byte*) malloc(size);
   if (addr) {
      while (ch = mgr->readByte(addr++)) {
         if (i == size) {
            str = (byte*)realloc(str, size + 16);
            size += 16;
         }
         str[i++] = ch;
      }
      if (i == size) {
         str = (byte*)realloc(str, size + 1);
      }
   }
   str[i] = 0;
   return (char*)str;
}

/*
 * Build an ascii C string by reading directly from the database
 * until a NULL is encountered.  Returned value must be free'd
 */

char *getString(dword addr) {
   int size = 16;
   int i = 0;
   byte *str = NULL, ch;
   str = (byte*) malloc(size);
   if (addr) {
      while (ch = get_byte(addr++)) {
         if (i == size) {
            str = (byte*)realloc(str, size + 16);
            size += 16;
         }
         if (ch == 0xFF) break;  //should be ascii, something wrong here
         str[i++] = ch;
      }
      if (i == size) {
         str = (byte*)realloc(str, size + 1);
      }
   }
   str[i] = 0;
   return (char*)str;
}

/*
 * set the callback function to use when anything that is hooked, but
 * unemulated is called
 */
void setUnemulatedCB(unemulatedCB cb) {
   unemulated_cb = cb;
}

/*
 * This function is used for all unemulated API functions
 */
void unemulated(MemoryManager *mgr, dword addr) {
   if (unemulated_cb) {
      HookNode *n = findHookByAddr(addr);
      (*unemulated_cb)(mgr, addr, n ? n->getName() : NULL);
   }
}

/*
   These functions emulate various API calls.  The idea is
   to invoke them after all parameters have been pushed onto the
   stack.  Each function understands its corresponding parameters
   and calling conventions and leaves the stack in the proper state
   with a result in eax.  Because these are invoked from the emulator
   no return address gets pushed onto the stack and the functions can
   get right at their parameters on top of the stack.
*/

void emu_CreateThread(MemoryManager *mgr, dword addr) {
   /*LPSECURITY_ATTRIBUTES lpThreadAttributes = */ pop(SIZE_DWORD);
   /*SIZE_T dwStackSize = */ pop(SIZE_DWORD);
   dword lpStartAddress = pop(SIZE_DWORD);
   dword lpParameter = pop(SIZE_DWORD);
   /*DWORD dwCreationFlags = */ pop(SIZE_DWORD);
   dword lpThreadId = pop(SIZE_DWORD);
   
   ThreadNode *tn = emu_create_thread(lpStartAddress, lpParameter);
   
   if (lpThreadId) {
      writeMem(lpThreadId, tn->id, SIZE_DWORD);
   }
   eax = tn->handle;
}

void emu_HeapCreate(MemoryManager *mgr, dword addr) {
   /* DWORD flOptions =*/ pop(SIZE_DWORD); 
   /* SIZE_T dwInitialSize =*/ pop(SIZE_DWORD);
   dword dwMaximumSize = pop(SIZE_DWORD);
   //we are not going to try to do growable heaps here
   if (dwMaximumSize == 0) dwMaximumSize = 0x01000000;
   eax = mgr->addHeap(dwMaximumSize);
}

void emu_HeapDestroy(MemoryManager *mgr, dword addr) {
   dword hHeap = pop(SIZE_DWORD); 
   eax = mgr->destroyHeap(hHeap);
}

void emu_GetProcessHeap(MemoryManager *mgr, dword addr) {
   eax = mgr->heap ? mgr->heap->getHeapBase() : 0;
}

void emu_HeapAlloc(MemoryManager *mgr, dword addr) {
   dword hHeap = pop(SIZE_DWORD); 
   /* DWORD dwFlags =*/ pop(SIZE_DWORD);
   dword dwBytes = pop(SIZE_DWORD);
   EmuHeap *h = mgr->findHeap(hHeap);
   //are HeapAlloc  blocks zero'ed?
   eax = h ? h->calloc(dwBytes, 1) : 0;
}

void emu_HeapFree(MemoryManager *mgr, dword addr) {
   dword hHeap = pop(SIZE_DWORD); 
   /* DWORD dwFlags =*/ pop(SIZE_DWORD);
   dword lpMem = pop(SIZE_DWORD);
   EmuHeap *h = mgr->findHeap(hHeap);
   eax = h ? h->free(lpMem) : 0;
}

void emu_VirtualAlloc(MemoryManager *mgr, dword addr) {
   /*dword lpAddress =*/ pop(SIZE_DWORD); 
   dword dwSize = pop(SIZE_DWORD);
   /*dword flAllocationType =*/ pop(SIZE_DWORD);
   /*dword flProtect =*/ pop(SIZE_DWORD);
   eax = mgr->heap->calloc(dwSize, 1);
}

void emu_VirtualFree(MemoryManager *mgr, dword addr) {
   eax = mgr->heap->free(pop(SIZE_DWORD));
   /*dword dwSize =*/ pop(SIZE_DWORD);
   /*dword dwFreeType =*/ pop(SIZE_DWORD);
}

void emu_LocalAlloc(MemoryManager *mgr, dword addr) {
   /*dword uFlags =*/ pop(SIZE_DWORD); 
   dword dwSize = pop(SIZE_DWORD);
   eax = mgr->heap->malloc(dwSize);
}

void emu_LocalFree(MemoryManager *mgr, dword addr) {
   eax = mgr->heap->free(pop(SIZE_DWORD));
}

//funcName should be a library function name, and funcAddr its address
hookfunc checkForHook(char *funcName, dword funcAddr, dword moduleId) {
   int i = 0;
   for (i = 0; hookTable[i].fName; i++) {
      if (!strcmp(hookTable[i].fName, funcName)) {
         //if there is an emulation, hook it
         return addHook(funcName, funcAddr, hookTable[i].func, moduleId);
      }
   }
   //there is no emulation, pass all calls to the "unemulated" stub
   return addHook(funcName, funcAddr, unemulated, moduleId);
}

dword myGetProcAddress(dword hModule, dword lpProcName, MemoryManager *mgr = NULL) {
   dword h = 0;
   char *procName = NULL;
   HandleList *m = findModuleByHandle(hModule);
   if (m == NULL) return 0;
   if (lpProcName < 0x10000) {
      //getting function by ordinal value
      char *dot;
      procName = (char*) malloc(strlen(m->moduleName) + 16);
      sprintf(procName, "%s_0x%4.4X", m->moduleName, m->handle);
      dot = strchr(procName, '.');
      if (dot) *dot = '_';
      if ((m->handle & FAKE_HANDLE_BASE) == 0) {
//            lpProcName -= m->ordinal_base;
         h = get_long(m->eat + lpProcName * 4) + m->handle;
      }
      else {
         //need a fake procaddress when faking module handle
         FakedImport *f = addFakedImport(m, procName);
         h = f->addr;
      }
   }
   else {
      //getting function by name
      if (mgr) {  // using MemoryManager, string may be in stack or heap
         procName = getString(mgr, lpProcName);
      }
      else { //pull string straight from database
         procName = getString(lpProcName);
      }
      if ((m->handle & FAKE_HANDLE_BASE) == 0) {
         //binary search through export table to match lpProcName
         int hi = m->NoN - 1;
         int lo = 0;
         while (lo <= hi) {
            int mid = (hi + lo) / 2;
            char *name = getString(get_long(m->ent + mid * 4) + m->handle);
            int res = strcmp(name, procName);
            if (res == 0) {
               free(name);
               lpProcName = get_word(m->eot + mid * 2); // - m->ordinal_base;
               h = get_long(m->eat + lpProcName * 4) + m->handle;
               break;
            }
            else if (res < 0) lo = mid + 1;
            else hi = mid - 1;
            free(name);
         }
      }
      else {
         //need a fake procaddress when faking module handle
         FakedImport *f = addFakedImport(m, procName);
         h = f->addr;
      }         
   }
   free(procName);
   return h;
}

//FARPROC __stdcall GetProcAddress(HMODULE hModule,LPCSTR lpProcName)
void emu_GetProcAddress(MemoryManager *mgr, dword addr) {
   static dword address = 0x80000000;
   static dword bad = 0xFFFFFFFF;
   dword hModule = pop(SIZE_DWORD); 
   dword lpProcName = pop(SIZE_DWORD);
   char *procName = NULL;
   int i;
   eax = myGetProcAddress(hModule, lpProcName, mgr);
   HandleList *m = findModuleByHandle(hModule);
   procName = reverseLookupExport(eax);
   msg("x86emu: GetProcAddress called: %s", procName);
   //first see if this function is already hooked
   if (findHookByAddr(eax) == NULL) {
      //this is where we need to check if auto hooking is turned on else if (autohook) {
      //if it wasn't hooked, see if there is an emulation for it
      //use h to replace "address" and "bad" below
      for (i = 0; hookTable[i].fName; i++) {
         if (!strcmp(hookTable[i].fName, procName)) {
            //if there is an emulation, hook it
            if (eax == 0) eax = address++;
            addHook(procName, eax, hookTable[i].func, m ? m->id : 0);
            break;
         }
      }
      if (hookTable[i].fName == NULL) {
         //there is no emulation, pass all calls to the "unemulated" stub
         if (eax == 0) eax = bad--;
         addHook(procName, eax, unemulated, m ? m->id : 0);
      }
   }
   msg(" (0x%X)\n", eax);
   free(procName);
}

/*
 * This is how we build import tables based on calls to 
 * GetProcAddress: create a label at addr from lastProcName.
 */

void makeImportLabel(dword addr, dword val) {
   for (dword cnt = 0; cnt < 4; cnt++) {
      do_unknown(addr + cnt, true); //undefine it
   }
   doDwrd(addr, 4);
   char *name = reverseLookupExport(val);
   if (name && !set_name(addr, name, SN_PUBLIC | SN_NOCHECK | SN_NOWARN)) { //failed, probably duplicate name
      //undefine old name and retry once
      dword oldName = get_name_ea(BADADDR, name);
      if (oldName != BADADDR && del_global_name(oldName)) {
         set_name(addr, name, SN_PUBLIC | SN_NOCHECK | SN_NOWARN);
      }
   }
   free(name);
}

HandleList *moduleCommon(MemoryManager *mgr, dword addr) {
   dword lpModName = pop(SIZE_DWORD);
   char *modName = getString(mgr, lpModName);
   HandleList *m = findModuleByName(modName);
   if (m) {
      free(modName);
   }
   else {
      m = addModule(modName, 0);
   }
   if (m) {
      msg(" called: %s (%X)\n", m->moduleName, m->handle);
   }
   return m;
}

/*
 * To do: Need to mimic actual GetModuleHandle
 *          add .dll extension if no extension provided
 *          return first occurrence if duplicate suffix
 */

//HMODULE __stdcall GetModuleHandleA(LPCSTR lpModuleName)
void emu_GetModuleHandle(MemoryManager *mgr, dword addr) {
   msg("x86emu: GetModuleHandle");
   HandleList *m = moduleCommon(mgr, addr);
   eax = m->handle;
}

//HMODULE __stdcall LoadLibraryA(LPCSTR lpLibFileName)
void emu_LoadLibrary(MemoryManager *mgr, dword addr) {
   msg("x86emu: LoadLibrary");
   HandleList *m = moduleCommon(mgr, addr);
   eax = m->handle;
}

void emu_malloc(MemoryManager *mgr, dword addr) {
   eax = mgr->heap->malloc(readDword(esp));
}

void emu_calloc(MemoryManager *mgr, dword addr) {
   eax = mgr->heap->calloc(readDword(esp), readDword(esp + 4));
}

void emu_realloc(MemoryManager *mgr, dword addr) {
   eax = mgr->heap->realloc(readDword(esp), readDword(esp + 4));
}

void emu_free(MemoryManager *mgr, dword addr) {
   mgr->heap->free(readDword(esp));
}

void doImports(MemoryManager *mgr, PETables &pe) {
   for (thunk_rec *tr = pe.imports; tr; tr = tr->next) {
      HandleList *m = addModule(tr->dll_name, 0);

      dword slot = tr->iat_base + pe.base;
      for (int i = 0; tr->iat[i]; i++, slot += 4) {
         dword fname = pe.base + tr->iat[i] + 2;
         dword f = 0;
         if (m->handle & FAKE_HANDLE_BASE) {
            f = slot;
         }
         else {  //need to deal with ordinals here
            f = myGetProcAddress(m->handle, fname);
//            reverseLookupExport((dword)f);
         }
         put_long(slot, f);
         if (f) {
            char *funcname = getString(fname);
            checkForHook(funcname, f, m->id);
            free(funcname);
         }
      }      
   }   
}

//okay to call for ELF, but module list should be empty
HandleList *moduleFromAddress(dword addr) {
   HandleList *hl, *result = NULL;
   for (hl = moduleHead; hl; hl = hl->next) {
//#ifdef CYGWIN
      if (addr < hl->maxAddr && addr >= hl->handle) {
         result = hl;
         break;
      }
/*
#else
      //Because MS does not include psapi stuff in Visual Studio
      dword min = 0;
      if (addr > min && addr < hl->maxAddr && addr >= hl->handle) {
         result = hl;
         min = hl->handle;
      }
#endif
*/
   }
   return result;
}

bool isModuleAddress(dword addr) {
   return moduleFromAddress(addr) != NULL;
}

int reverseLookupFunc(dword EAT, dword func, dword max, dword base) {
   for (dword i = 0; i < max; i++) {
      if ((get_long(EAT + i * 4) + base) == func) return (int)i;
   }
   return -1;
}

int reverseLookupOrd(dword EOT, word ord, dword max) {
   for (dword i = 0; i < max; i++) {
      if (get_word(EOT + i * 2) == ord) return (int)i;
   }
   return -1;
}

//need to add fake_list check for lookups that have been faked
char *reverseLookupExport(dword addr) {
   HandleList *hl;
   char *fname = NULL;
   for (hl = moduleHead; hl; hl = hl->next) {
      if (addr < hl->maxAddr && addr >= hl->handle) break;
   }
   if (hl) {
      if (hl->handle & FAKE_HANDLE_BASE) {
         FakedImport *f = findFakedImportByAddr(hl, addr);
         if (f) {
            fname = strdup(f->name);
         }
      }
      else {
         int idx = reverseLookupFunc(hl->eat, addr, hl->NoF, hl->handle);
         if (idx != -1) {
            idx = reverseLookupOrd(hl->eot, idx, hl->NoN);
            if (idx != -1) {
               fname = getString(get_long(hl->ent + idx * 4) + hl->handle);
   //            msg("x86emu: reverseLookupExport: %X == %s\n", addr, fname);
            }
         }
      }
   }
   return fname;
}

void clearFunctionInfoList() {
   FunctionInfo *f;
   while (functionInfoList) {
      f = functionInfoList;
      functionInfoList = functionInfoList->next;
      free(f->fname);
      free(f);
   }
   functionInfoList = NULL;
}

FunctionInfo *getFunctionInfo(char *name) {
   FunctionInfo *f;
   for (f = functionInfoList; f; f = f->next) {
      if (!strcmp(name, f->fname)) break;
   }
   return f;
}

void addFunctionInfo(char *name, dword result, dword nitems, dword callType) {
   FunctionInfo *f;
   for (f = functionInfoList; f; f = f->next) {
      if (!strcmp(name, f->fname)) break;
   }
   if (f == NULL) {
      f = (FunctionInfo*)malloc(sizeof(FunctionInfo));
      f->fname = (char*)malloc(strlen(name) + 1);
      strcpy(f->fname, name);
      f->next = functionInfoList;
      functionInfoList = f;
   }
   f->result = result;
   f->stackItems = nitems;
   f->callingConvention = callType;
}

void saveFunctionInfo(Buffer &b) {
   int count = 0;
   FunctionInfo *f;
   for (f = functionInfoList; f; f = f->next) count++;
   b.write(&count, sizeof(count));
   for (f = functionInfoList; f; f = f->next) {
      count = strlen(f->fname) + 1;  //account for null
      b.write(&count, sizeof(count));
      b.write(f->fname, count);  //note this writes the null
      b.write(&f->result, sizeof(f->result));
      b.write(&f->stackItems, sizeof(f->stackItems));
      b.write(&f->callingConvention, sizeof(f->callingConvention));
   }
}

void loadFunctionInfo(Buffer &b) {
   int count = 0, len;
   FunctionInfo *f;
   clearFunctionInfoList();
   b.read(&count, sizeof(count));
   for (; count; count--) {
      f = (FunctionInfo*)malloc(sizeof(FunctionInfo));
      b.read(&len, sizeof(len));
      f->fname = (char*)malloc(len);
      b.read(f->fname, len);
      b.read(&f->result, sizeof(f->result));
      b.read(&f->stackItems, sizeof(f->stackItems));
      b.read(&f->callingConvention, sizeof(f->callingConvention));
      f->next = functionInfoList;
      functionInfoList = f;
   }
}

