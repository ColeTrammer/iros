#include <kernel/hal/timer.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/proc/task.h>

void register_callback(void (*callback)(), unsigned int ms) {
    pit_register_callback(callback, ms);
}

void set_sched_callback(void (*callback)(struct task_state*), unsigned int ms) {
    pit_set_sched_callback(callback, ms);
}

struct timespec get_time_resolution() {
    return (struct timespec) { .tv_sec = 0, .tv_nsec = 1000000 };
}