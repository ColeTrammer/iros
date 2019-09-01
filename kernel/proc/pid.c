#include <sys/types.h>
#include <stdio.h>

#include <kernel/proc/pid.h>
#include <kernel/hal/output.h>

static pid_t counter = 1;

pid_t get_next_pid() {
    debug_log("PID Assigned: %d\n", counter + 1);
    return counter++;
}

void free_pid(pid_t pid) {
    printf("Free PID: %d\n", pid);
}