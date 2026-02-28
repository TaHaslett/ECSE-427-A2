#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "scripts.h"
#include "shell.h"
#include "shellmemory.h"
#include "scheduler.h"

bool scheduler_running = false;
ScriptQueue *scheduler_queue = NULL;

// multi threading variables
bool mt_mode_enabled = false;
pthread_t worker_threads[NUM_WORKERS];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
bool workers_should_stop = false;
int mt_time_slices;

// tracks how many scripts are currently being executed by a worker 
// (dequeued but not yet re-enqueued or freed). Used so the main thread 
// knows the queue is not truly empty while a worker holds a script.
static int scripts_in_flight = 0;

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

void *worker_thread_func(void *arg) {
    (void)arg;

    while(1) {
        pthread_mutex_lock(&queue_mutex);
        // wait until there is a script in the queue or until the workers should stop
        while (is_empty_script_queue(scheduler_queue) && !workers_should_stop) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        // exit the thread if signaled to stop and there are no more scripts to process
        if (workers_should_stop && is_empty_script_queue(scheduler_queue) && scripts_in_flight == 0) {
            pthread_mutex_unlock(&queue_mutex);
            break; // exit the thread if signaled to stop and there are no more scripts to process
        }

        // if woken up but the queue is empty, release the lock and continue waiting
        if (is_empty_script_queue(scheduler_queue)) {
            pthread_mutex_unlock(&queue_mutex);
            continue; // if the queue is empty, release the lock and wait for the next signal
        }
        Script *script = dequeue_script(scheduler_queue);
        scripts_in_flight++; // increment scripts_in_flight since we just dequeued a script that will now be processed by this worker
        pthread_mutex_unlock(&queue_mutex);

        for (int i = 0; i < mt_time_slices; i++) { // run the script for its time slices
            parseInput(script_memory[script->start_idx + script->pc]);
            script->pc++;
            if (script->pc >= script->length) {
                break;
            }
        }
        // reinsert the script back into the queue if it is not finished after its time slices
        pthread_mutex_lock(&queue_mutex);
        scripts_in_flight--; // decrement scripts_in_flight since this worker is done processing the script for now, even if it is not finished and will be re-enqueued
        if (script->pc >= script->length) {
            free(script);
        } else {            
            enqueue_script(scheduler_queue, script);
        }
        pthread_cond_broadcast(&queue_cond); // signal the main thread in case it is waiting for the queue to be empty
        pthread_mutex_unlock(&queue_mutex);
    }
    return NULL;
}

void start_worker_threads(int time_slices) {
    mt_time_slices = time_slices;
    workers_should_stop = false;
    scripts_in_flight = 0;
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_create(&worker_threads[i], NULL, worker_thread_func, NULL);
    }
}

void stop_worker_threads() {
    pthread_mutex_lock(&queue_mutex);
    workers_should_stop = true;
    pthread_cond_broadcast(&queue_cond); // wake up all worker threads so they can exit
    pthread_mutex_unlock(&queue_mutex);
    for (int i = 0; i < NUM_WORKERS; i++) { // wait for all worker threads to finish
        pthread_join(worker_threads[i], NULL);
    }
}

int scheduler(Policy policy, Script *script1, Script *script2, Script *script3, Script *batch_script, bool multi_thread_mode) {
    bool outermost_scheduler = false;
    int errCode = 0;

    if (!scheduler_running) {
        scheduler_running = true;
        outermost_scheduler = true;
        scheduler_queue = create_script_queue();
    }

    if (multi_thread_mode && !mt_mode_enabled) {
        mt_mode_enabled = true;
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
    if (!outermost_scheduler) {
        if (mt_mode_enabled) {
            pthread_mutex_lock(&queue_mutex);
            pthread_cond_broadcast(&queue_cond); // signal worker threads in case they are waiting for new scripts to be added to the queue
            pthread_mutex_unlock(&queue_mutex);
        }
        return 0;
    }

    if (mt_mode_enabled && (policy == RR || policy == RR30)) {
        int time_slices = (policy == RR) ? 2 : 30;
        
        if (batch_script != NULL && !is_empty_script_queue(scheduler_queue) && peek_script(scheduler_queue)->pid == batch_script->pid) {
            Script *batch_script_in_queue = peek_script(scheduler_queue);
            for (int i = 0; i < time_slices; i++) {
                parseInput(script_memory[batch_script_in_queue->start_idx + batch_script_in_queue->pc]);
                batch_script_in_queue->pc++;
                if (batch_script_in_queue->pc >= batch_script_in_queue->length) {
                    free(batch_script_in_queue);
                    batch_script_in_queue = NULL;
                    break;
                }
            }
            if (batch_script_in_queue != NULL) {
                enqueue_script_front(scheduler_queue, batch_script_in_queue); // reinsert the batch script at the front of the queue if it is not finished after its time slices
            }
        }

        start_worker_threads(time_slices);

        // signal the workers that there are scripts to work on
        pthread_mutex_lock(&queue_mutex);
        pthread_cond_broadcast(&queue_cond);
        // wait for queue to be empty before stopping the worker threads
        while (!is_empty_script_queue(scheduler_queue) || scripts_in_flight > 0) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        pthread_mutex_unlock(&queue_mutex);

        // stop the worker threads after the queue is empty
        stop_worker_threads();
    } else { // normal execution for non-MT modes
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
    }
    scheduler_running = false;
    free(scheduler_queue);
    scheduler_queue = NULL;
    return errCode;
}