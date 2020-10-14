#include <kernel/hal/processor.h>
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
    wait_for_with_spinlock(get_current_task(), !!queue, &wait_queue, &queue_lock);
    struct task *ret = queue;
    assert(ret);
    queue = ret->finialize_queue_next;
    spin_unlock(&queue_lock);
    return ret;
}

static void finalizer_task_entry() {
    for (;;) {
        free_task(take_from_queue(), true);
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
