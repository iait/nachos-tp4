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
#include "synch.h"
#include "system.h"

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

//Parece que esto es codigo viejo
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

// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(const char* debugName) 
{
    name = debugName;
    sem = new Semaphore(debugName, 1);
    owner = NULL;
}

Lock::~Lock() 
{
    delete sem;
}

void func(Thread *t) { scheduler->Promote(t); } //Para llamar apply
void Lock::Acquire() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(!isHeldByCurrentThread());
// Esta parte es para arreglar el problema de inversión de prioridades
// la solución que aplicamos es mover el hilo de menor prioridad y toda
// la cola del lock que está en espera a la cola del thread de mayor prioridad
// que pide el lock. Luego volverá a su cola original al hacer Release
    int currentPriority = currentThread->getPriority();
    if (owner != NULL && owner->getPriority() < currentPriority){
        scheduler->Promote(owner);
        sem->getQueue()->Apply(func);
    }
//
    sem->P();
    owner = currentThread;
    interrupt->SetLevel(oldLevel);
}

void Lock::Release() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(isHeldByCurrentThread());

    // Se restaura la prioridad original del hilo en ejecución
    currentThread->setPriority(currentThread->getOriginalPriority());

    owner = NULL;
    sem->V();
    interrupt->SetLevel(oldLevel);
}

bool Lock::isHeldByCurrentThread()
{
    return owner == currentThread;
}

Condition::Condition(const char* debugName, Lock* conditionLock) 
{
    name = debugName;
    lock = conditionLock;
    sems = new List<Semaphore*>;
}

Condition::~Condition() 
{ 
    delete sems;
}

void Condition::Wait() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    Semaphore *sem = new Semaphore(currentThread->getName(), 0);
    sems->Append(sem);
    lock->Release();
    sem->P();
    lock->Acquire();
    delete sem;

    interrupt->SetLevel(oldLevel);
}

void Condition::Signal() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(lock->isHeldByCurrentThread());
    Semaphore *sem = sems->Remove();
    if (sem != NULL) {
        sem->V();
    }

    interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    while(!sems->IsEmpty()) {
        Signal();
    }

    interrupt->SetLevel(oldLevel);
}
