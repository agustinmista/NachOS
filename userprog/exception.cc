// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "readwrite.h"
#include "filesys.h"
#include "synchconsole.h"
#include "addrspace.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

#define MAX_FILENAME_LENGTH 128
#define SC_OK  0
#define SC_ERROR -1

char **SaveArgs(int address) {
    ASSERT(address != 0);

    // Contamos la cantidad de argumentos hasta NULL.
    int val;
    unsigned i = 0;
    do {
        machine->ReadMem(address + i * 4, 4, &val);
        i++;
    } while (i < MAX_ARG_COUNT && val != 0);
    if (i == MAX_ARG_COUNT && val != 0)
        // Se llegó al máximo de argumentos pero el último no es NULL.
        // Devolvemos NULL como error.
        return NULL;

    DEBUG('a', "Saving %u command line arguments from parent process.\n", i);

    // Alocamos un arreglo de i punteros. Sabemos que i siempre va a valer al
    // menos 1.
    char **ret = new char * [i];
    // Para cada puntero leemos la cadena correspondiente.
    for (unsigned j = 0; j < i - 1; j++) {
        ret[j] = new char [MAX_ARG_LENGTH];
        machine->ReadMem(address + j * 4, 4, &val);
        ReadStringFromUser(val, ret[j], MAX_ARG_LENGTH);
    }
    // Escribimos el último NULL.
    ret[i - 1] = NULL;

    return ret;
}


// Incrementa los registros necesarios
// luego de ejecutar una instruccion
void incrementPCRegs() {
    int pc = machine->ReadRegister(PCReg);
    machine->WriteRegister(PrevPCReg, pc);
    
    pc = machine->ReadRegister(NextPCReg);
    machine->WriteRegister(PCReg, pc);
    machine->WriteRegister(NextPCReg, pc + 4);
}


// Incicia un proceso, y escribe sus argumentos
// en el stack del userspace del mismo
void startProc(void *args) {
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    currentThread->space->WriteArgs((char **) args);
    machine->Run();
}
 

// Maneja una excepcion de tipo Syscall
void handleSyscall() {
     
    int type = machine->ReadRegister(2); 
     
    switch (type) {
            
        case SC_Halt:
        {
            /*
             * void Halt()
             *
             * Interrumpe la ejecución y apaga el sistema.
             */
            DEBUG('a', "!!!!! Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;
        }

        case SC_Exit:
        {
            /*
             *  void Exit(int status)
             *
             *  Finaliza un proceso, con valor de retorno status.
             */
            int eCode = machine->ReadRegister(4);
            DEBUG('a', "***** Thread finished with status code: %d\n", eCode);
            currentThread->exitCode = eCode;
            currentThread->Finish();
            break;
        }
            
        case SC_Exec:
        {
            /* 
             *  int Exec(char *path, int prio, char **argv)
             *
             *  Crea un proceso, y ejecuta el binario presente
             *  en path, con prioridad prio y los argumentos
             *  argv. Devuelve el pid del proceso hijo, o
             *  SC_ERROR en otro caso.
             */
            int addr = machine->ReadRegister(4); 
            int prio = machine->ReadRegister(5); 
            int args_addr = machine->ReadRegister(6); 
            
            char path[MAX_FILENAME_LENGTH];
            ReadStringFromUser(addr, path, MAX_FILENAME_LENGTH);

            OpenFile *bin = fileSystem->Open(path);

            if (bin) {
                char **args = SaveArgs(args_addr);
                
                Thread *binThread = new Thread(path, prio);

                binThread->Fork(startProc, args);
                SpaceId pid = newThread(binThread);
 
                AddrSpace *binSpace = new AddrSpace(bin, pid);
                binThread->space = binSpace;
                
                ASSERT(pid != -1);

                machine->WriteRegister(2, pid);
                DEBUG('a', "***** Executing binary %s with pid: %d\n", path, pid);
            } else {
                DEBUG('a', "***** Failed opening executable \"%s\"\n", path);
                machine->WriteRegister(2, SC_ERROR);
            }

            break;
        }
            
        case SC_Join:
        {
            /*
             *  int Join(SpaceId id)
             *
             *  Espera a que un hilo/proceso pid termine
             *  y devuelve el valor de retorno del mismo.
             *  Falla con SC_ERROR si el proceso no es valido.
             *  Se asume que SC_ERROR no es un valor valido de retorno.
             */       
            SpaceId pid = (SpaceId) machine->ReadRegister(4);
            Thread *thread = getThread(pid); //system.cc
            
            if (thread) {
                DEBUG('a', "***** Joining pid: %d\n", (int) pid);
                machine->WriteRegister(2, thread->Join()); //hago return con lo que devuelve t.
                removeThread(pid);
            } else {
                DEBUG('a', "***** Invalid pid to join: %d\n", (int) pid);
                machine->WriteRegister(2, SC_ERROR);
            }
            break;

        }
            
        case SC_Create:
        {
            /*
             *  int Create(char *name)
             *
             *  Crea un archivo, devuelve SC_OK si todo va bien
             *  o SC_ERROR si el archivo no se pudo crear
             */
            int arg1 = machine->ReadRegister(4);
            char name[MAX_FILENAME_LENGTH];
            ReadStringFromUser(arg1, name, MAX_FILENAME_LENGTH);

            if(fileSystem->Create(name, 0)){
                DEBUG('a', "+++++ New File %s\n", name);
                machine->WriteRegister(2, SC_OK);
            } else {
                DEBUG('a', "+++++ Error creating file %s\n", name);
                machine->WriteRegister(2, SC_ERROR);
            }
            
            break; 
         }
            
        case SC_Open:
        {
            /*
             *  OpenFileId Open(char *name)
             *
             *  Abre un archivo, devuelve el fd si todo bien,
             *  o SC_ERROR si ocurrio un error.
             */
            int fname = machine->ReadRegister(4);
            char name[MAX_FILENAME_LENGTH];
            ReadStringFromUser(fname, name, MAX_FILENAME_LENGTH);
            
            OpenFile *file = fileSystem->Open(name);
            
            if (file) {
                OpenFileId fd = currentThread->getFileDescriptor(file);
                if (fd >= 0){
                    DEBUG('a', "+++++ Opened file %s: %d\n", name, fd);
                    machine->WriteRegister(2, fd);
                } else {
                    DEBUG('a', "+++++ Error opening file %s\n", name);
                    machine->WriteRegister(2, SC_ERROR);
                }
            } else {
				DEBUG('a', "+++++ Error opening file %s\n", name);
				machine->WriteRegister(2, SC_ERROR);
			}
            break;
        }
            
        case SC_Read:
        {
            /*
             *  int Read(char *buffer, int size, OpenFileId id)
             *
             *  Lee desde un archivo o consola, retorna la cantidad
             *  real de caracteres leidos, o SC_ERROR si ocurre un
             *  error.
             */
            int dest = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            OpenFileId fd = (OpenFileId) machine->ReadRegister(6);     
            
            char data[size];
            
            // Leemos desde consola
            if (fd == ConsoleInput) {
                
                for(int i=0; i<size; i++) data[i] = synchedConsole->SynchGetChar();
				
				WriteBufferToUser(data, dest, size);
				machine->WriteRegister(2, size);
                break;
            }
            
            // Sino, leemos desde un archivo
            OpenFile *file = currentThread->getOpenFile(fd);
            if (file) { 
                int read = file->Read(data, size);
                DEBUG('a', "+++++ %d bytes read from fd %d\n", read, fd);
                WriteBufferToUser(data, dest, read);
                machine->WriteRegister(2, read);
            } else {
                DEBUG('a', "+++++ Trying to read from invalid fd: %d\n", fd);
                machine->WriteRegister(2, SC_ERROR);
            }

            break;
        }
            
        case SC_Write:
        {
            /*
             *  int Write(char *buffer, int size, OpenFileId id)
             *
             *  Escribe en un archivo abierto, o en la consola.
             *  Retorna la cantidad de caracteres escritos o 
             *  SC_ERROR si se intenta escribir en ConsoleInput
             *  o un fd invalido.
             */
            OpenFileId fd = (OpenFileId) machine->ReadRegister(6);     
            int size = machine->ReadRegister(5);     
            int source = machine->ReadRegister(4);     
    
            char data[size];
            ReadBufferFromUser(source, data, size);
		    
            // Escribimos a la consola
            if (fd == ConsoleOutput) {
                for(int i=0; i<size; i++) synchedConsole->SynchPutChar(data[i]);
				machine->WriteRegister(2, size);
                break;
            }
            
            // Sino, escribimos a un archivo
            OpenFile *file = currentThread->getOpenFile(fd);
            if (file) { 
                int written = file->Write(data, size);
                DEBUG('a', "+++++ %d bytes written to fd %d\n", written, fd);
                machine->WriteRegister(2, written);
            } else {
                DEBUG('a', "+++++ Trying to write on invalid fd: %d\n", fd);
                machine->WriteRegister(2, SC_ERROR);
            }
            break;
        }
            
        case SC_Close:
        {
            /*
             *  int Close(OpenFileId id)
             *
             *  Cierra un archivo, retorna SC_OK si todo bien, o
             *  SC_ERROR si estaba cerrado o es un I/O estandar.
             */
            OpenFileId fd = (OpenFileId) machine->ReadRegister(4);

			OpenFile *file = currentThread->getOpenFile(fd);
			
			if (file){
				delete file;
				currentThread->freeFileDescriptor(fd);
				DEBUG('a', "+++++ Closing file fd : %d\n", fd);
                machine->WriteRegister(2, SC_OK);
			} else {
                DEBUG('a', "+++++ Trying to close already closed file: %s\n", fd);
                machine->WriteRegister(2, SC_ERROR);
            }
            break;
        }

        default:
        {
            DEBUG('a', "!!!!! Unexpected syscall exception %d\n", type);
            break;
        }
    }
}

#ifdef USE_TLB
static int entry = 0;  //Índice de la proxima página a usar
#endif

void handlePageFault(){

    int badVaddr = machine->ReadRegister(BadVAddrReg);   //Dirección virtual que falló
    unsigned int vpn = badVaddr/PageSize;
    if (!currentThread->space->CheckVPN(vpn)){
        DEBUG('a', "----- VPN not in current thread space\n");
        currentThread->Finish();
    }
    
#ifdef VM
    TranslationEntry *faultPage = currentThread->space->GetEntry(vpn);
    if (faultPage->physicalPage == -1) {
        DEBUG('a',"----- Loading page %d from swap file\n", vpn);
        int frame = coremap->Find(currentThread->space, faultPage);
        faultPage->physicalPage = frame;
        currentThread->space->SwapIn(vpn, faultPage->physicalPage); 
    }
#endif

#ifdef USE_TLB    
    if(machine -> tlb[entry].valid)
        currentThread->space->SaveEntry(machine->tlb[entry]);
    machine->tlb[entry] = *(currentThread->space->GetEntry(vpn));
    DEBUG('a', "----- TLB swap happened, victim page: %d\n", entry); 
    //Ver bien como elegirlas
    entry = (entry+1) % TLBSize;
#endif
}

void ExceptionHandler(ExceptionType which) {

    switch (which) {
        case SyscallException:
            handleSyscall();
            incrementPCRegs(); 
            break;
        case PageFaultException:
            #ifdef USE_TLB
            handlePageFault();
            #endif
            break;
        case ReadOnlyException: //?
            DEBUG('a', "----- Write attempt on read-only page\n");
            currentThread->Finish();
            break;
        default: 
            printf("!!!!! Unexpected user mode exception %d\n", which);
            ASSERT(false);
    }
}
