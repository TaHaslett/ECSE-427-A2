#ifndef SCRIPTS_H
#define SCRIPTS_H

typedef struct Script {
    int pid;
    int start_idx;
    int length;
    int pc;
    struct Script *next;
} Script;

typedef struct {
    Script *head;
    Script *tail;
} ScriptQueue;

ScriptQueue *create_script_queue();
Script *create_script(int start, int length);
int is_empty_script_queue(ScriptQueue *queue);
int enqueue_script(ScriptQueue *queue, Script *script);
Script *dequeue_script(ScriptQueue *queue);
Script *peek_script(ScriptQueue *queue);

#endif