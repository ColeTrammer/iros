#include <kernel/proc/task.h>
#include <kernel/proc/task_finalizer.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>

static struct task *queue;
static spinlock_t queue_lock = SPINLOCK_INITIALIZER;
static struct wait_queue wait_queue = WAIT_QUEUE_INITIALIZER;

static struct task *take_from_queue(void) {
    spin_lock(&queue_lock);
    struct task *ret = queue;
    if (ret) {
        queue = ret->finialize_queue_next;
    }
    spin_unlock(&queue_lock);
    return ret;
}

static void finalizer_task_entry() {
    for (;;) {
        struct task *task;
        while ((task = take_from_queue())) {
            free_task(task, true);
        }

        wait_on(&wait_queue);
    }
}

void proc_schedule_task_for_destruction(struct task *task) {
    spin_lock(&queue_lock);
    task->finialize_queue_next = queue;
    queue = task;
    wake_up_all(&wait_queue);
    spin_unlock(&queue_lock);
}

void init_task_finalizer(void) {
    sched_add_task(load_kernel_task((uintptr_t) &finalizer_task_entry, "finalizer"));
}
