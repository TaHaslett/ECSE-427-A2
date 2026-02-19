#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"

int batch = 0;

int parseInput(char ui[]);

// Start of everything
int main(int argc, char *argv[]) {
    printf("Shell version 1.5 created Dec 2025\n");

    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// user's input stored here
    int errorCode = 0;					// zero means no error, default

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }

    if (!isatty(fileno(stdin))) batch = 1;

    //init shell memory
    mem_init();
    while(1) {							
        if (!batch) printf("%c ", prompt);
        if (fgets(userInput, MAX_USER_INPUT-1, stdin) == NULL) {
	        break;
	    }
        errorCode = parseInput(userInput);
        if (errorCode == -1) exit(99);	// ignore all other errors
        memset(userInput, 0, sizeof(userInput));
    }

    return 0;
}

int wordEnding(char c) {
    // You may want to add ';' to this at some point,
    // or you may want to find a different way to implement chains.
    return c == '\0' || c == '\n' || c == ' ' || c == ';';
}

int parseInput(char inp[]) {
    char tmp[200], *words[100];                            
    int ix = 0, w = 0;
    int wordlen = 0;
    int errorCode = 0;
    while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000) {
        while (inp[ix] == ' ' && ix < 1000) ix++; // skip white spaces
        
        wordlen = 0;
        while (!wordEnding(inp[ix]) && ix < 1000) {
            tmp[wordlen++] = inp[ix++];  
        }

        if (wordlen > 0) {
            tmp[wordlen] = '\0';
            words[w++] = strdup(tmp);
        }

        // check char for command terminator
        if (inp[ix] == ';') {
            errorCode = interpreter(words, w);
            w = 0;
            ix++;
        } else if (inp[ix] == '\0' || inp[ix] == '\n') {
            break;
        } else {
            ix++;
        }

    }
    if (w > 0) {
        errorCode = interpreter(words, w);
    }
    return errorCode;
}
