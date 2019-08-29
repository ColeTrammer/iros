#include <sys/types.h>
#include <stdio.h>

#include <kernel/proc/pid.h>

static pid_t counter = 1;

pid_t get_next_pid() {
    return counter++;
}

void free_pid(pid_t pid) {
    printf("Free PID: %d\n", pid);
}