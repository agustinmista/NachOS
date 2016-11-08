#include "copyright.h"

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "console.h"
#include "synch.h"

// This class defines a synchronous console interaction.

class SynchConsole {

	public:
		SynchConsole(const char *inFile, const char *outFile);
		~SynchConsole();
		void SynchPutChar(char c);
		char SynchGetChar();
	
	private:
		Console *console;
		Lock *rlock;
		Lock *wlock;
		Semaphore *readAvail;
		Semaphore *writeDone;
		static void SynchReadAvail(void *);
		static void SynchWriteDone(void *);

};

#endif
