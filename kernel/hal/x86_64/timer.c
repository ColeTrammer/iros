#include <kernel/arch/x86_64/proc/task.h>
#include <kernel/hal/timer.h>
#include <kernel/hal/x86_64/drivers/pit.h>

void register_callback(void (*callback)(), unsigned int ms) {
    pit_register_callback(callback, ms);
}

void set_sched_callback(void (*callback)(struct task_state*), unsigned int ms) {
    pit_set_sched_callback(callback, ms);
}

time_t get_time() {
    return pit_get_time();
}