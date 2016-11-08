#include "copyright.h"
#include "synchconsole.h"

SynchConsole::SynchConsole(const char *inFile, const char *outFile){
	console = new Console(inFile, 
                          outFile, 
                          SynchConsole::SynchReadAvail,
                          SynchConsole::SynchWriteDone,
                          this
                          );
	
    readAvail = new Semaphore("read available", 0);
	writeDone = new Semaphore("write done", 0);
	
	wlock = new Lock ("write lock");
	rlock = new Lock ("read lock");	
}


SynchConsole::~SynchConsole(){
	
	delete console;
	delete readAvail;
	delete writeDone;
	delete wlock;
	delete rlock;
	
}

void
SynchConsole::SynchPutChar(char c){
	
	wlock->Acquire();
	console->PutChar(c);
	writeDone->P();
	wlock->Release();
	
}

char
SynchConsole::SynchGetChar(){

	rlock->Acquire();
	readAvail->P();
	char c = console->GetChar();
	rlock->Release();
	return c;
	
}

void
SynchConsole::SynchReadAvail(void *cons){
	((SynchConsole *)cons)->readAvail->V();
}

void
SynchConsole::SynchWriteDone(void *cons){
	((SynchConsole *)cons)->writeDone->V();
}
