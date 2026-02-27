#include "scripts.h"
#include "shell.h"
#include "shellmemory.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void sort_scripts_by_length(Script *scripts[], int size) {
    // Simple bubble sort based on script length
    bool swapped;
    for (int i = 0; i < size - 1; i++) {
        swapped = false;
        for (int j = 0; j < size - i - 1; j++) {
            if (scripts[j] != NULL && scripts[j + 1] != NULL && scripts[j]->length > scripts[j + 1]->length) {
                Script *temp = scripts[j];
                scripts[j] = scripts[j + 1];
                scripts[j + 1] = temp;
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

int scheduler(Policy policy, Script *script1, Script *script2, Script *script3) {
    int errCode = 0;
    ScriptQueue *queue = create_script_queue();
    bool is_preemptive_policy;
    switch (policy) {
    case FIFO:
        enqueue_script(queue, script1);
        enqueue_script(queue, script2);
        enqueue_script(queue, script3);
        is_preemptive_policy = false;
        break;
    
    case SJF:
        Script *scripts[3] = {script1, script2, script3};
        sort_scripts_by_length(scripts, 3);
        for (int i = 0; i < 3; i++) {
            if (scripts[i] != NULL) {
                enqueue_script(queue, scripts[i]);
            }
        }
        is_preemptive_policy = false;
        break;

    case RR:
        break;

    case AGING:
        break;

    default:
        break;
    }
    while(!is_empty_script_queue(queue)) {
        if (!is_preemptive_policy) {
            errCode = parseInput(script_memory[peek_script(queue)->start_idx + peek_script(queue)->pc++]);
            if (peek_script(queue)->pc >= peek_script(queue)->length) {
                dequeue_script(queue);
            }
        
        } else {
            // Implement preemptive scheduling logic here
        }
    }
    free(queue);
    return errCode;
}