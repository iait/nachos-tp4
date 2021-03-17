// port.h
//      Implementación de puertos.

#ifndef PORT_H
#define PORT_H

#include "synch.h"

//--------------------------------------------------------------------------
//    Envío de mensajes a 'puertos' que permiten que los emisores se sincronicen
//    con los receptores
//
//    Send(int msg)     -- espera hasta que se llama a receive y envía msg
//    Receive(int *msg) -- espera hasta que se llama a send y recibe el msg
//---------------------------------------------------------------------------
class Port
{
    public:
        Port(const char * debugName);
        ~Port();
        void Send(int msg);
        void Receive(int *msg);
    private:
        const char *name;
        int message;
        Lock *senderLock; // para que haya un solo emisor a la vez
        Lock *receiverLock; // para que haya un solo receptor a la vez
        Lock *commonLock; // para pasar el mensaje
        Condition *availableCond; // para avisar cuando haya mensaje disponible
        Condition *receivedCond; // para avisar cuando se recibió el mensaje
        bool available; // indica si hay mensaje disponible
};

#endif // PORT_H
