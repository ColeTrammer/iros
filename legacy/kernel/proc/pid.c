#include <stdio.h>
#include <sys/types.h>

#include <kernel/hal/output.h>
#include <kernel/proc/pid.h>
#include <kernel/util/spinlock.h>

// #define PID_DEBUG

// Initial kernel proc gets 1
static pid_t counter = 2;
static spinlock_t pid_lock = SPINLOCK_INITIALIZER;

pid_t get_next_pid() {
#ifdef PID_DEBUG
    debug_log("PID Assigned: [ %d ]\n", counter);
#endif /* PID_DEBUG */

    spin_lock(&pid_lock);

    pid_t ret = counter++;

    spin_unlock(&pid_lock);

    return ret;
}

void free_pid(pid_t pid) {
    (void) pid;
#ifdef PID_DEBUG
    debug_log("Free PID: [ %d ]\n", pid);
#endif /* PID_DEBUG */
}
