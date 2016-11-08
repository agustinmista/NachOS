#include "syscall.h"


#define MAX_LINE_SIZE  60
#define MAX_ARG_COUNT  32
#define ARG_SEPARATOR  ' '

#define NULL  ((void *) 0)

static inline unsigned strlen(const char *s)
{

    if(!s) return 0;
    unsigned i;
    for (i=0; s[i] != '\0'; i++);
    return i;
}

static inline void WritePrompt(OpenFileId output)
{
    static const char PROMPT[] = "--> ";
    Write(PROMPT, sizeof PROMPT - 1, output);
}

static inline void WriteError(const char *description,
                              OpenFileId  output)
{
    static const char PREFIX[] = "Error: ";
    static const char SUFFIX[] = "\n";
	static const char UERROR[] = "Unknown";
    
    Write(PREFIX, sizeof PREFIX - 1, output);
    if(description) 	
    	Write(description, strlen(description), output);
    else
    	Write(UERROR, strlen(UERROR), output);
    
    Write(SUFFIX, sizeof SUFFIX - 1, output);
}

static unsigned ReadLine(char      *buffer,
                         unsigned   size,
                         OpenFileId input)
{
    if(!buffer) return 0;
    unsigned i;

    for (i = 0; i < size; i++) {
        Read(&buffer[i], 1, input);
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            break;
        }
    }
    return i;
}

static int PrepareArguments(char *line, char  **argv, unsigned argvSize) {
    
    unsigned argCount, i;
    char *lp = line;

    if (!line || !argv) return -1; 
    
    argv[0] = lp;
    argCount = 1;

    // Se recorre toda la línea y se reemplazan los espacios entre argumentos
    // por caracteres nulos para que se pueda tratar cada argumento como una
    // cadena en sí misma.
    for (i=0; lp[i] != '\0'; i++)
        if (lp[i] == ARG_SEPARATOR) {
            
            if (argCount == argvSize - 1) return -1;
            
            lp[i] = '\0'; 
                        
            while (lp[i+1] == ARG_SEPARATOR) i++;
            
            argv[argCount] = &lp[i + 1];
            argCount++;
        }

    argv[argCount] = NULL;
    return 0;
}

int main(void) {
    const OpenFileId INPUT  = ConsoleInput;
    const OpenFileId OUTPUT = ConsoleOutput;
    char             line[MAX_LINE_SIZE], *lp;
    char            *argv[MAX_ARG_COUNT];
    int background;
    unsigned i;

    for (;;) {
        WritePrompt(OUTPUT);
        background = 0;

        for (i=0; i<MAX_LINE_SIZE; i++) line[i] = '\0';

        const unsigned lineSize = ReadLine(line, MAX_LINE_SIZE, INPUT);
        if (lineSize == 0) continue;
        lp = line;
        
        while (*lp == ARG_SEPARATOR) lp++;
        if (lp[0]=='&') {
            background = 1;
            lp++;    
        } 

        while (*lp == ARG_SEPARATOR) lp++;
        if (PrepareArguments(lp, argv, MAX_ARG_COUNT) != 0) {
            WriteError("error parsing command", OUTPUT);
            continue;
        }
        
        const SpaceId newProc = Exec(lp, 5, argv);
        if (newProc == -1) {
            Write(lp, strlen(lp), OUTPUT);       
            Write(": command not found\n", strlen(": command not found\n"), OUTPUT);       
        } else if (!background) Join(newProc);

    }

    return 0;  // Nunca se alcanza.
}
