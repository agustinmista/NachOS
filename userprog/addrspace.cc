// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "readwrite.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable, int id) {
    pid = id;
    
    NoffHeader noffH;
    unsigned int size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

#ifdef VM
    char swapFilename[32];
    sprintf(swapFilename, "SWAP.%d", pid);
    DEBUG('a', "----- Creating swap file: %s\n", swapFilename);
    ASSERT(fileSystem->Create(swapFilename, MemorySize));
    swap = fileSystem->Open(swapFilename);
#endif


// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

#ifndef VM
    ASSERT(numPages <= NumPhysPages);
#endif

    DEBUG('a', "----- Initializing address space, num pages %d, size %d\n", 
			numPages, size);
// first, set up the translation
    pageTable = new TranslationEntry[numPages];
    
    for (unsigned int i=0; i < numPages; i++) {
	    pageTable[i].virtualPage = i;
#ifndef VM
        pageTable[i].physicalPage = memPages->Find();
        ASSERT(pageTable[i].physicalPage != -1);
#else 
        pageTable[i].physicalPage = coremap->Find(this, GetEntry(i));
#endif
        pageTable[i].valid = true;
	    pageTable[i].use = false;
	    pageTable[i].dirty = false;
	    pageTable[i].readOnly = false;  
        
        bzero(&(machine->mainMemory[pageTable[i].physicalPage*PageSize]), PageSize);
    }
    
    char byte;
    int addr, frame, offset;

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "----- Initializing code segment, at 0x%x, size %d\n", 
                noffH.code.virtualAddr, noffH.code.size);
        for (int i=0; i<noffH.code.size; i++){
            
            executable->ReadAt(&byte, 1, noffH.code.inFileAddr+i);
            addr = noffH.code.virtualAddr + i;
            frame = pageTable[addr/PageSize].physicalPage;
#ifndef VM
            ASSERT(frame >= 0);
#else 
            if (frame < 0) {
                frame = coremap->Find(this, GetEntry(addr/PageSize));
                SwapIn(addr/PageSize, frame);
            }
#endif
            offset = addr%PageSize;
            machine->mainMemory[frame*PageSize+offset] = byte;
        }
    
    }

    if (noffH.initData.size > 0) {
        DEBUG('a', "----- Initializing data segment, at 0x%x, size %d\n", 
                noffH.initData.virtualAddr, noffH.initData.size);
        for (int i=0; i<noffH.initData.size; i++){
            executable->ReadAt(&byte, 1, noffH.initData.inFileAddr+i);
            addr = noffH.initData.virtualAddr + i;
            frame = pageTable[addr/PageSize].physicalPage;
#ifndef VM
            ASSERT(frame >= 0);
#else 
            if (frame < 0) {
                frame = coremap->Find(this, GetEntry(addr/PageSize));
                SwapIn(addr/PageSize, frame);
            }
#endif
            offset = addr%PageSize;
            machine->mainMemory[frame*PageSize+offset] = byte;
        }
    
    }

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    for(unsigned int i=0; i<numPages; i++) 
#ifndef VM
        memPages->Clear(pageTable[i].physicalPage);
#else
        coremap->Clear(pageTable[i].physicalPage);
#endif
    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "----- Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    #ifdef USE_TLB
    //También se podrían guardar las entradas válidas para volver a
    //cargarlas a la vuelta
    for (int i = 0;i < TLBSize; i++){
        if(machine->tlb[i].valid)
            currentThread->space->pageTable[machine->tlb[i].virtualPage] = machine->tlb[i];
    }
    #endif

}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    
    #ifdef USE_TLB
    for (int i = 0; i < TLBSize; i++)
        machine->tlb[i].valid = false;
    #else
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
    #endif
}

// Escribe los argumentos args en el espacio de memoria del proceso.
void AddrSpace::WriteArgs(char **args) {
    
    ASSERT(args != NULL);
    DEBUG('a', "----- Writing command line arguments into child process.\n");

    // Comenzamos a escribir los argumentos donde está el SP actual.
    int args_address[MAX_ARG_COUNT];
    int sp = machine->ReadRegister(StackReg);
    
    unsigned argc;
    for (argc = 0; argc < MAX_ARG_COUNT; argc++) {
        // Si llegamos al último, terminamos.
        if (args[argc] == NULL)
            break;
        // Bajamos el SP (dejando un byte para el \0).
        sp -= strlen(args[argc]) + 1;
        // Escribimos la cadena allí.
        WriteStringToUser(args[argc], sp);
        
        // Nos guardamos la dirección del argumento.
        args_address[argc] = sp;
        // Liberamos la memoria.
        //
    DEBUG('a', "----- Writing command line argument %d at %d.\n",argc,sp);
        delete args[argc];
    }
    ASSERT(argc < MAX_ARG_COUNT);

    // Alineamos la pila a múltiplo de cuatro.
    sp -= sp % 4;
    // Hacemos lugar para el arreglo y el último NULL.
    sp -= argc * 4 + 4;
    for (unsigned j = 0; j < argc; j++)
        // Guardamos la dirección del argumento j-ésimo desde atrás hacia
        // adelante.
        machine->WriteMem(sp + 4 * j, 4, args_address[j]);
    // El último es NULL.
    machine->WriteMem(sp + 4 * argc, 4, 0);
    // Dejamos lugar para los “register saves”.
    machine->WriteRegister(5, sp);       /* char **argv */
    sp -= 16;

    machine->WriteRegister(4, argc);     /* int argc */
    machine->WriteRegister(StackReg, sp);
    
    // Liberamos el arreglo.
    delete args;
}

#ifdef VM
//----------------------------------------------------------------------
// AddrSpace::SwapOut
//----------------------------------------------------------------------
void AddrSpace::SwapOut(int vpn) {
    int phys = pageTable[vpn].physicalPage;
    
    // enviamos la página a disco
    swap->WriteAt(&(machine->mainMemory[phys*PageSize]), PageSize, vpn*PageSize);

    // marcamos la página como inválida en la pageTable
    pageTable[vpn].physicalPage = -1;

#ifdef USE_TLB
    // Si éste es el proceso actual, debo invalidar la entrada en la tlb
    if(currentThread->space == this)
        for(int i=0; i<TLBSize; i++)
            if((machine->tlb[i].virtualPage == vpn) && machine->tlb[i].valid) 
                machine->tlb[i].valid = false;
#endif
    
    DEBUG('a',"----- Page %d swapped to disk\n", vpn);
}

//----------------------------------------------------------------------
// AddrSpace::SwapIn
//----------------------------------------------------------------------
void AddrSpace::SwapIn(int vpn, int physPage) {
    pageTable[vpn].physicalPage = physPage;
    swap->ReadAt(&(machine->mainMemory[physPage*PageSize]), PageSize, vpn*PageSize);
    
    DEBUG('a',"----- Page %d loaded from disk into frame %d\n", vpn, physPage);
}
#endif

TranslationEntry *AddrSpace::GetEntry(int index){
    ASSERT(CheckVPN(index));
    return &pageTable[index];
}
#ifdef USE_TLB
void AddrSpace::SaveEntry(TranslationEntry entry){
    ASSERT(CheckVPN(entry.virtualPage));
    pageTable[entry.virtualPage] = entry;
}
#endif
