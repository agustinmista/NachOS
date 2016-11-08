// portTest.cc 
//	Simple port test case based on producer-consumers problem

#include "copyright.h"
#include "system.h"

#include "synch.h"

Puerto *port = new Puerto("test");

void prod(void *name){
    for (int i = 0; i<100; i++){
        port->Send(i);
        DEBUG('p', "Hilo %s manda %d \n", (char*)name,i);
    }
}

void cons(void *name){
    int m;
    while(1){
        port->Recv(&m);
        DEBUG('p', "Hilo %s recibe %d \n", (char*)name,m);
    }
}

void ThreadTest(){
    
  Thread *Productor = new Thread ("Productor");
  Productor->Fork(prod, (void*)"Productor");
  cons((void*) "Consumidor");

}
