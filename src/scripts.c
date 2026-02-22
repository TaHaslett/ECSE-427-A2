#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include "scripts.h"

typedef struct {
    pid_t pid;
    int start_idx;
    int end_idx;
    int curr_idx;
} Script;

typedef struct {
    Script script;
    ScriptQueueNode *next;
    ScriptQueueNode *prev;    
} ScriptQueueNode;

typedef struct {
    ScriptQueueNode *head;
    ScriptQueueNode *tail;
} ScriptQueue;

char script_memory[MEM_SIZE][LINE_SIZE];

ScriptQueue *create_script_queue() {
    ScriptQueue *q = (ScriptQueue *)malloc(sizeof(ScriptQueue));	
    if (q == NULL) {
    	perror("Could not allocate ScriptQueue");
	exit(EXIT_FAILURE);
    }
    q->head = q->tail = NULL;
    return q;
}

Script *create_script() {
    Script *s = (Script *)malloc(sizeof(Script));
    if (s == NULL) {
        perror("Could not allocate Script");
	exit(EXIT_FAILURE);
    }
    return s;
}

ScriptQueueNode *create_script_queue_node(Script script) {
    ScriptQueueNode *n = (ScriptQueueNode *)malloc(sizeof(ScriptQueueNode);
    if (n == NULL) {
    	perror("Could not allocate ScriptQueueNode");
	exit(EXIT_FAILURE);
    }
    return n;
}	

void enqueue_script(ScriptQueue *q, Script *script) {
}	

Script *dequeue_script() {
}

Script *peek_script() {

}
