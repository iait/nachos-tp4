// port.cc
//      Implementación de puertos.

#include "port.h"

Port::Port(const char *debugName)
{
    name = debugName;
    available = false;
    senderLock = new Lock(debugName);
    receiverLock = new Lock(debugName);
    commonLock = new Lock(debugName);
    availableCond = new Condition(debugName, commonLock);
    receivedCond = new Condition(debugName, commonLock);
}

Port::~Port()
{
    delete senderLock;
    delete receiverLock;
    delete availableCond;
    delete receivedCond;
    delete commonLock;
}

void Port::Send(int msg)
{
    senderLock->Acquire();

    commonLock->Acquire();
    message = msg;
    available = true;
    availableCond->Signal();
    while (available) {
        receivedCond->Wait();
    }
    commonLock->Release();

    senderLock->Release();
}

void Port::Receive(int *msg)
{
    receiverLock->Acquire();

    commonLock->Acquire();
    while (!available) {
        availableCond->Wait();
    }
    *msg = message;
    available = false;
    receivedCond->Signal();
    commonLock->Release();

    receiverLock->Release();
}

