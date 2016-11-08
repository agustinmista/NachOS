#include "syscall.h"

#define NULL  ((void *) 0)
#define MAX_ARGS 5

unsigned strlen(const char *s)
{
    unsigned i;
    for (i = 0; s[i] != '\0'; i++);
    return i;
}

int main(int argc, char **argv) {
    
    if (argc < 2 || argc > MAX_ARGS) {
        Write("cat: invalid args count\n", strlen("cat: invalid args count\n"), ConsoleOutput);         
        return -1;
    }
    
    int i;
    int readed;
    OpenFileId src;
    char buffer[100];

    for(i=1; i < argc; i++){
    	src = Open(argv[i]);

    	if (src == -1) {
        	Write("cat: invalid source file [", strlen("cat: invalid source file ["), ConsoleOutput);
                Write(argv[i], strlen(argv[i]), ConsoleOutput);
                Write("]\n", strlen("]\n"), ConsoleOutput);         
    	} else {
    	    while((readed = Read(buffer, 100, src)) != 0)
                Write(buffer, readed, ConsoleOutput);
        }
    
    	Close(src);
    }

    return 0;
}
