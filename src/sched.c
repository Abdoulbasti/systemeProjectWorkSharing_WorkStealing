#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "sched.h"

int sched_init(int nthreads, int qlen, taskfunc f, void *closure){
    return 1;
}



int sched_spawn(taskfunc f, void *closure, struct scheduler *s){
    return 1;
}