// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "bitmap.h"
#include "machine.h"

#define UserStackSize		1024 	// increase this as necessary!

const unsigned MAX_ARG_COUNT  = 32;
const unsigned MAX_ARG_LENGTH = 128;


class AddrSpace {
  public:
    AddrSpace(OpenFile *executable, int pid);	// Create an address space,
	                    				// initializing it with the program
					                    // stored in the file "executable"
    ~AddrSpace();			            // De-allocate an address space

    void InitRegisters();		        // Initialize user-level CPU registers,
                                        // before jumping to user code
    void SaveState();			        // Save/restore address space-specific
    void RestoreState();		        // info on a context switch 
    void WriteArgs(char **args);        // Escribe los argumentos de argv en el stack
    
    inline bool CheckVPN(unsigned int vpn){ return (vpn >=0 && vpn < numPages); }
    TranslationEntry *GetEntry(int index); 
#ifdef USE_TLB
    void SaveEntry(TranslationEntry entry); //guarda una entrada victima de la tlb #def
    //Chequea si una página está dentro del límite del proceso
#endif

#ifdef VM
    void SwapIn(int vpn, int physPage);
    void SwapOut(int vpn);
#endif

    TranslationEntry *pageTable;	
  
  private:
    int pid;
    unsigned int numPages;		// Number of pages in the virtual 
					            // address space
#ifdef VM
    OpenFile *swap;
#endif
};

#endif // ADDRSPACE_H
