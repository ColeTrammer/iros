#include <sys/types.h>
#include <stdio.h>

#include <kernel/proc/pid.h>
#include <kernel/hal/output.h>
#include <kernel/util/spinlock.h>

// Initial kernel proc gets 1
static pid_t counter = 2;
static spinlock_t pid_lock = SPINLOCK_INITIALIZER;

pid_t get_next_pid() {
    debug_log("PID Assigned: [ %d ]\n", counter);
    
    spin_lock(&pid_lock);

    pid_t ret = counter++;

    spin_unlock(&pid_lock);

    return ret;
}

void free_pid(pid_t pid) {
    debug_log("Free PID: [ %d ]\n", pid);
}