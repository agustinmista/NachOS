// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(const char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List<Thread*>;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append(currentThread);		// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    interrupt->SetLevel(oldLevel);		// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    interrupt->SetLevel(oldLevel);
}


//----------------------------------------------------------------------
// Lock::Lock 
//----------------------------------------------------------------------
Lock::Lock(const char* debugName) {
    sem = new Semaphore(debugName, 1);
    name = debugName;
    acquiredBy = NULL;
}

//----------------------------------------------------------------------
// Lock::~Lock 
//----------------------------------------------------------------------
Lock::~Lock() {
    delete sem;
}

//----------------------------------------------------------------------
// Lock::Acquire
//----------------------------------------------------------------------
void Lock::Acquire() {
    ASSERT(!isHeldByCurrentThread());
    sem->P();
    acquiredBy = currentThread;
}

//----------------------------------------------------------------------
// Lock::Release
//----------------------------------------------------------------------
void Lock::Release() {
    ASSERT(isHeldByCurrentThread());
    acquiredBy = NULL;
    sem->V();
}

//----------------------------------------------------------------------
// Lock::isHeldByCurrentThread
//----------------------------------------------------------------------
bool Lock::isHeldByCurrentThread(){
    return currentThread == acquiredBy;
}


//----------------------------------------------------------------------
// Condition::Condition
//----------------------------------------------------------------------
Condition::Condition(const char* debugName, Lock* conditionLock){
    name = debugName;
    myLock = conditionLock;
    sem_queue = new List <Semaphore *> ();
}

//----------------------------------------------------------------------
// Condition::~Condition
//----------------------------------------------------------------------
Condition::~Condition(){
    delete myLock;
    delete sem_queue;
}

//----------------------------------------------------------------------
// Condition::Wait
//----------------------------------------------------------------------
void Condition::Wait(){
    Semaphore *s = new Semaphore(name, 0);
    sem_queue -> Append(s);
    
    myLock -> Release();
    s -> P();
    myLock -> Acquire();
    delete s;
}

//----------------------------------------------------------------------
// Condition::Signal
//----------------------------------------------------------------------
void Condition::Signal(){
    if(!sem_queue->IsEmpty()){
        Semaphore *s = sem_queue -> Remove();
        s->V();
   }
}

//----------------------------------------------------------------------
// Condition::Broadcast
//----------------------------------------------------------------------
void Condition::Broadcast(){
    while(!sem_queue->IsEmpty()){
        Semaphore *s = sem_queue -> Remove();
        s->V();
    }
}

//----------------------------------------------------------------------
// Puerto::Puerto
//----------------------------------------------------------------------
Puerto::Puerto(const char* debugName){
    name = debugName;
    myLock = new Lock(debugName);
    sendCond = new Condition(debugName, myLock);
    recvCond = new Condition(debugName, myLock);
    empty = true;
}

//----------------------------------------------------------------------
// Puerto::~Puerto
//----------------------------------------------------------------------
Puerto::~Puerto(){
    delete myLock;
    delete sendCond;
    delete recvCond;
}

//----------------------------------------------------------------------
// Puerto::Send
//----------------------------------------------------------------------
void Puerto::Send(int mensaje){
    
    myLock->Acquire();
    
    while(!empty) sendCond->Wait();
    
    msg = mensaje;
    empty = !empty;
    recvCond->Signal();
    
    myLock->Release();
    
}

//----------------------------------------------------------------------
// Puerto::Recv
//----------------------------------------------------------------------
void Puerto::Recv(int *destiny){

    myLock->Acquire();
    
    while(empty) recvCond->Wait();
    
    *destiny = msg;
    empty = !empty;
    sendCond->Signal();

    myLock->Release();
}
