#ifndef SCHEDULER_H
#define SCHEDULER_H

typedef enum {
    FCFS,
    SJF,
    RR,
    RR30,
    AGING
} Policy;

int scheduler(Policy policy, Script *script1, Script *script2, Script *script3);

#endif