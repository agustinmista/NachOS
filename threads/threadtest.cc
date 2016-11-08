// threadtest.cc
//	Simple test case for the myThreads assignment.
//
//	Create several myThreads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
//
// Parts from Copyright (c) 2007-2009 Universidad de Las Palmas de Gran Canaria
//

#include "copyright.h"
#include "system.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 10 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"name" points to a string with a thread name, just for
//      debugging purposes.
//----------------------------------------------------------------------

void
SimpleThread(void* name)
{
    // Reinterpret arg "name" as a string
    char* threadName = (char*)name;

    // If the lines dealing with interrupts are commented,
    // the code will behave incorrectly, because
    // printf execution may cause race conditions.
    for (int num = 0; num < 10; num++) {
        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
	    printf(KYEL "*** thread %s looped %d times\n" RESET, threadName, num);
	    //interrupt->SetLevel(oldLevel);
        currentThread->Yield();
    }
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(KGRN ">>> Thread %s has finished\n" RESET, threadName);
    //interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between several myThreads, by launching
//	ten myThreads which call SimpleThread, and finally calling
//	SimpleThread ourselves.
//----------------------------------------------------------------------




void
ThreadTest()
{   
    
    DEBUG('t', "Entering SimpleTest");

    int threadsNumber = 5;
    Thread *myThreads[threadsNumber];


    for (int i=0; i < threadsNumber; i++) {
        int p = 9-i;
        
        char *name = new char[8];
        sprintf(name, "H%dP%d", i, p);

        myThreads[i] = new Thread (name, p);
        myThreads[i]->Fork (SimpleThread, (void*)name);
    }

    currentThread->Yield();
    
    // Esto podria imprimirse mientras los hijos siguen en ejecucion
    printf(KRED "Los hijos aun estan corriendo\n" RESET);

    for (int i=0; i < threadsNumber; i++)
        myThreads[i]->Join();

    // Esto debería imprimirse solo cuando terminen los hijos
    printf(KRED "Los hijos ya deberian haber terminado\n" RESET);

}
