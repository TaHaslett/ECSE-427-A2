#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "scripts.h"

int generate_pid() {
    static int pid = 0; // persists across function calls
    return pid++;
}

ScriptQueue *create_script_queue() {
    ScriptQueue *queue = malloc(sizeof(ScriptQueue));
    if (queue == NULL) {
        perror("Could not allocate ScriptQueue");
        exit(EXIT_FAILURE);
    }
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

Script *create_script(int start, int length) {
    Script *s = malloc(sizeof(Script));
    if (s == NULL) {
        perror("Could not allocate Script");
        exit(EXIT_FAILURE);
    }
    s->pid = generate_pid();
    s->start_idx = start;
    s->length = length;
    s->pc = 0;
    s->next = NULL;
    s->job_length_score = length; // initialize job_length_score to the length of the script
    return s;
}

int is_empty_script_queue(ScriptQueue *queue) {
    if (queue == NULL) {
        return 1; // consider a NULL queue as empty
    }
    return queue->head == NULL;
}

int enqueue_script(ScriptQueue *queue, Script *script) {
    if (queue == NULL)
        return -1;
    if (script == NULL)
        return -1;
    script->next = NULL; // ensure the new script's next is NULL
    if (is_empty_script_queue(queue)) {
        queue->head = script;
        queue->tail = script;
    } else {
        queue->tail->next = script;
        queue->tail = script; 
    }
    return 0;
}	

Script *dequeue_script(ScriptQueue *queue) {
    if (is_empty_script_queue(queue)) return NULL;
    Script *script = queue->head;
    queue->head = script->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    script->next = NULL; // detach the dequeued script from the queue
    return script;
}

Script *peek_script(ScriptQueue *queue) {
    if (queue == NULL) return NULL;
    return queue->head;
}

int aging_enqueue_script(ScriptQueue *queue, Script *script) {
    if (queue == NULL)
        return -1;
    if (script == NULL)
        return -1;

    script->next = NULL; // ensure the new script's next is NULL

    if (is_empty_script_queue(queue)) {
        queue->head = script;
        queue->tail = script;
    } else {
        // insert the script based on its job_length_score
        Script *current = queue->head;
        Script *previous = NULL;

        while (current != NULL && current->job_length_score < script->job_length_score) {
            previous = current;
            current = current->next;
        }

        if (previous == NULL) { // insert at the head
            script->next = queue->head;
            queue->head = script;
        } else { // insert in the middle or end
            previous->next = script;
            script->next = current;
            if (current == NULL) { // update tail if inserted at the end
                queue->tail = script;
            }
        }
    }
    return 0;
}

int enqueue_script_front(ScriptQueue *queue, Script *script) {
    if (queue == NULL)
        return -1;
    if (script == NULL)
        return -1;
    script->next = NULL; // ensure the new script's next is NULL
    if (is_empty_script_queue(queue)) {
        queue->head = script;
        queue->tail = script;
    } else {
        script->next = queue->head;
        queue->head = script;
    }
    return 0;
}