#include "readwrite.h"

//----------------------------------------------------------
//  ReadStringFromUser
//----------------------------------------------------------
void ReadStringFromUser(int usrAddr, char *outStr, unsigned byteCount) {
    unsigned i = 0;
    int c;
    
    do {
        if (!machine->ReadMem(usrAddr+i, 1, &c))
          ASSERT(machine->ReadMem(usrAddr+i,1,&c));
        outStr[i++] = (char) c;
    } while(c !='\0' && i<byteCount);
}

//----------------------------------------------------------
//  ReadBufferFromUser
//----------------------------------------------------------
void ReadBufferFromUser(int usrAddr, char *outBuff, unsigned byteCount){
    int c;
    for(unsigned i=0; i<byteCount; i++){
       if (!machine->ReadMem(usrAddr+i, 1, &c))
          ASSERT(machine->ReadMem(usrAddr+i,1,&c));
       outBuff[i] = (char) c;
    }
}

//----------------------------------------------------------
//  WriteStringToUser
//----------------------------------------------------------
void WriteStringToUser(char *str, int usrAddr){
    unsigned i = 0;
    char c;
    
    do {
        c = str[i];
        if (!machine->WriteMem(usrAddr+i, 1, c))
          ASSERT(machine->WriteMem(usrAddr+i, 1, c));
        i++;
    } while(c !='\0');
}


//----------------------------------------------------------
//  WriteBufferToUser
//----------------------------------------------------------
void WriteBufferToUser(char *str, int usrAddr, unsigned byteCount){
    char c;
    for(unsigned i=0; i<byteCount; i++){
        c = str[i];
        if (!machine->WriteMem(usrAddr+i, 1, c))
          ASSERT(machine->WriteMem(usrAddr+i, 1, c));
    }
}

