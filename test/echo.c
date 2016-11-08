#include "syscall.h"

#define MAX_LINE_SIZE  60
#define MAX_ARG_COUNT  32

#define NULL  ((void *) 0)

unsigned strlen(const char *s)
{
    unsigned i;
    for (i = 0; s[i] != '\0'; i++);
    return i;
}

int main(int argc, char **argv) {
    
    if (argc > 0) {
        int i;
        for (i=1; i<argc; i++) {
            Write(argv[i], strlen(argv[i]), ConsoleOutput);
            Write(" ", strlen(" "), ConsoleOutput);
        }
        Write("\n", strlen("\n"), ConsoleOutput);
    }
    return 0;
}
