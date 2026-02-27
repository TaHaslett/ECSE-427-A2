#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "scripts.h"
#include "shell.h"
#include "shellmemory.h"
#include "scheduler.h"

bool scheduler_running = false;
ScriptQueue *scheduler_queue = NULL;

void sort_scripts_by_length(Script *scripts[], int size) {
    // Simple bubble sort based on script length
    for (int i = 0; i < size - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < size - i - 1; j++) {
            if (scripts[j] == NULL && scripts[j + 1] != NULL) {
                Script *temp = scripts[j];
                scripts[j] = scripts[j + 1];
                scripts[j + 1] = temp;
                swapped = true;
            } else if ( 
                    scripts[j] != NULL && 
                    scripts[j + 1] != NULL && 
                    scripts[j]->length > scripts[j + 1]->length) {
                
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

int non_preemptive_execute(ScriptQueue *queue) {
    int errCode = 0;
    while (!is_empty_script_queue(queue)) {
        errCode = parseInput(script_memory[peek_script(queue)->start_idx + peek_script(queue)->pc]);
        peek_script(queue)->pc++;
        if (peek_script(queue)->pc >= peek_script(queue)->length) {
            Script *script = dequeue_script(queue);
            free(script);
        }
    }
    return errCode;
}

int round_robin(ScriptQueue *queue, int time_slices) {
    int errCode = 0;
    while (!is_empty_script_queue(queue)) {
        Script *current_script = dequeue_script(queue);
        for (int i = 0; i < time_slices; i++) { // run each script for time slices
            errCode = parseInput(script_memory[current_script->start_idx + current_script->pc]);
            current_script->pc++;
            if (current_script->pc >= current_script->length) {
                free(current_script);
                current_script = NULL;
                break;
            }
        }
        // if the script is not finished after its time slices, move it to the back of the queue
        if (current_script != NULL) {
            enqueue_script(queue, current_script);
        }
    }
    return errCode;
}

int aging(ScriptQueue *queue) {
    int errcode = 0;
    while (!is_empty_script_queue(queue)) {
        Script *script = dequeue_script(queue); // remove the script at the front of the queue
        errcode = parseInput(script_memory[script->start_idx + script->pc]);
        script->pc++;
        // update the job_length_score of all scripts in the queue
        Script *current = queue->head;
        while (current != NULL) {
            if (current->job_length_score > 0) {
                current->job_length_score--; // decrease the job_length_score of all scripts in the queue
            }
            current = current->next;
        }
        if (script->pc >= script->length) {
            free(script);
        } else {
            aging_enqueue_script(queue, script); // reinsert the script based on its updated job_length_score
        }
    }
    return errcode;
}

int scheduler(Policy policy, Script *script1, Script *script2, Script *script3, Script *batch_script) {
    bool outermost_scheduler = false;
    int errCode = 0;
    if (!scheduler_running) {
        scheduler_running = true;
        outermost_scheduler = true;
        scheduler_queue = create_script_queue();
    }
    Script *scripts[3] = {script1, script2, script3};
    
    if (policy == SJF) {
        sort_scripts_by_length(scripts, 3); 
        for (int i = 0; i < 3; i++) {
            if (scripts[i] != NULL) {
                enqueue_script(scheduler_queue, scripts[i]);
            }
        }
    } else if (policy == AGING) {
        // add scripts to the queue in reverse order so that they are initially processed in the order they were given if there is a tie in their job_length_score, 
        // but will be re-ordered based on their job_length_score as they are processed
        // this is the behavior that was laid out in the assignment spec
        for (int i = 2; i >= 0; i--) {
            if (scripts[i] != NULL) {
                aging_enqueue_script(scheduler_queue, scripts[i]);
            }
        }
    } else { // all other policies just add scripts to the queue in the order they were given
        for (int i = 0; i < 3; i++) {
            if (scripts[i] != NULL) {
                enqueue_script(scheduler_queue, scripts[i]);
            }
        }
    }
    
    // a pointer to NULL indicates that the exec command is not being run in background mode
    if (batch_script != NULL) {
            enqueue_script_front(scheduler_queue, batch_script); 
    }

    // only execute the scheduler if this is the outermost scheduler call
    // nested scheduler calls will add their scripts to the queue, but the outermost call will be responsible for executing them
    if (outermost_scheduler) {
        switch (policy) {
            case FCFS:
            case SJF:
                errCode = non_preemptive_execute(scheduler_queue);
                break;

            case RR:
                errCode = round_robin(scheduler_queue, 2);
                break;
            case RR30:
                errCode = round_robin(scheduler_queue, 30);
                break;
            case AGING:
                errCode = aging(scheduler_queue);
                break;
            default:
                break;
        }
        scheduler_running = false;
        free(scheduler_queue);
        scheduler_queue = NULL;
    }
    return errCode;
}