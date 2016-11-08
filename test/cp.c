#include "syscall.h"

#define NULL  ((void *) 0)

unsigned strlen(const char *s)
{
    unsigned i;
    for (i = 0; s[i] != '\0'; i++);
    return i;
}

int main(int argc, char **argv) {
    
    if (argc != 3) {
        Write("cp: invalid args count\n", strlen("cp: invalid args count\n"), ConsoleOutput);         
        return -1;
    }

    OpenFileId src = Open(argv[1]);

    if (src == -1) {
        Write("cp: invalid source file\n", strlen("cp: invalid source file\n"), ConsoleOutput);         
        return -1;
    }

    int res_cre = Create(argv[2]);
    
    if (res_cre == -1) {
        Write("cp: error creating destination file\n", strlen("cp: error creating destination file\n"), ConsoleOutput);         
        return -1;
    }

    OpenFileId dst = Open(argv[2]);
    if (dst == -1) {
        Write("cp: error opening destination file\n", strlen("cp: error opening destination file\n"), ConsoleOutput);         
        return -1;
    }

    int readed;
    char buffer[100];
    while((readed = Read(buffer, 100, src)) != 0)
        Write(buffer, readed, dst);
    
    Close(src);
    Close(dst);

    return 0;
}
