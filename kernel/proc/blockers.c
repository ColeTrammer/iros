#include <assert.h>

#include <kernel/hal/timer.h>
#include <kernel/proc/blockers.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

static bool sleep_milliseconds_blocker(struct block_info *info) {
    assert(info->type == SLEEP_MILLISECONDS);
    return get_time() >= info->sleep_milliseconds_info.end_time;
}

void proc_block_sleep_milliseconds(struct task *current, time_t end_time) {
    disable_interrupts();
    current->block_info.sleep_milliseconds_info.end_time = end_time;
    current->block_info.type = SLEEP_MILLISECONDS;
    current->block_info.should_unblock = &sleep_milliseconds_blocker;
    current->blocking = true;
    current->sched_state = WAITING;
    __kernel_yield();
}