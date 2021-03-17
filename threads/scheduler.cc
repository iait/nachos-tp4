// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{
    for (int i = 0; i <= 10; i++) {
    	readyListP[i] = new List<Thread*>; 
    }
    readyList = readyListP[5]; //Para mantener compatibilidad con el código original.

    finishedThreads = new List<Thread*>;
    lock = new Lock("join");
    condition = new Condition("join", lock);
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{
    for (int i = 0; i <= 10; i++) {
        delete readyListP[i];
    }

    delete finishedThreads;
    delete lock;
    delete condition;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    thread->setStatus(READY);

// modificado para que agregue a la cola de la prioridad correspondiente
    readyListP[thread->getPriority()]->Append(thread);

}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------
//
// Modificado para que busque el siguiente por prioridad
//
Thread *
Scheduler::FindNextToRun ()
{
    for (int i = 10; i >= 1; i--) {
        if (!(readyListP[i]->IsEmpty())) {
            return readyListP[i]->Remove();
        }
    }
    
    return readyListP[0]->Remove();
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);
    
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------

static void
ThreadPrint(Thread *t) {
  t->Print();
}

void
Scheduler::Print()
{
    printf("Ready lists contents:\n");
    for (int i = 10; i <= 0; i--) {
        printf("Imprimiendo lista prioridad %d:\n", i);
        readyListP[i]->Apply(ThreadPrint);
        printf("\n");
    }
}

//------------------------------------------------------------------------
// Scheduler::Promote
//    Promueve al hilo thread a la prioridad del hilo actual.
//------------------------------------------------------------------------
void Scheduler::Promote(Thread *thread)
{
    ASSERT(currentThread->getPriority() > thread->getPriority());

    List<Thread*> *list = readyListP[thread->getPriority()];

    thread->setPriority(currentThread->getPriority());
    if (thread->getStatus() != READY) {
        return;
    }

    list->Remove(thread);
    ReadyToRun(thread);
}

//------------------------------------------------------------------------
// Scheduler::Finish
//      Agrega el hilo a una lista de terminados esperando por join.
//      Desbloquea a los hilos que ya llamaron join.
//------------------------------------------------------------------------
void Scheduler::Finish(Thread *thread)
{
    lock->Acquire();
    finishedThreads->Append(thread);
    condition->Broadcast();
    lock->Release();
}

//------------------------------------------------------------------------
// Scheduler::Join
//      Busca en la lista si el hilo thread ya terminó.
//      Si todavía no terminó, se bloquea este hilo (currentThread) a la espera
//      de que termine.
//      Una vez que el hilo terminó, lo destruye y continúa.
//------------------------------------------------------------------------
void Scheduler::Join(Thread *thread)
{
    lock->Acquire();
    while (!finishedThreads->Remove(thread)) {
        condition->Wait();
    }
    lock->Release();
    delete thread;
}

