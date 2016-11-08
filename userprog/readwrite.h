/*
 * Funciones para leer y escribir string y bytes desde userspace
*/
#ifndef READWRITE_H
#define READWRITE_H

#include "machine.h"
#include "system.h"
#include "copyright.h"

/*
 * Lee un string (hasta /0) desde userspace y lo almacena en outStr
*/
void ReadStringFromUser(int usrAddr, char *outStr, unsigned byteCount);

/*
 * Lee byteCount bytes desde userspace y lo almacena en outBuff
*/
void ReadBufferFromUser(int usrAddr, char *outBuff, unsigned byteCount);

/*
 * Escribe el string alojado en str en userspace  
*/
void WriteStringToUser(char *str, int usrAddr);

/*
 * Escribe byteCount bytes alojados en str en userspace 
*/
void WriteBufferToUser(char *str, int usrAddr, unsigned byteCount);

#endif //READWRITE_H
