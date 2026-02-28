#ifndef SCHEDULER_H
#define SCHEDULER_H

#define NUM_WORKERS 2

extern bool scheduler_running;
extern ScriptQueue *scheduler_queue;

extern bool mt_mode_enabled;
extern pthread_t worker_threads[NUM_WORKERS];
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t queue_cond;
extern bool workers_should_stop;
extern int mt_time_slices;

typedef enum {
    FCFS,
    SJF,
    RR,
    RR30,
    AGING,
    INVALID_POLICY
} Policy;

int scheduler(Policy policy, Script *script1, Script *script2, Script *script3, Script *batch_script, bool multi_thread_mode);
void start_worker_threads(int time_slices);
void stop_worker_threads();

#endif