#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/proc/task.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/sched/task_sched.h>

// #define WAIT_QUEUE_DEBUG

void init_wait_queue_internal(struct wait_queue *queue, const char *func) {
    (void) func;
    init_spinlock(&queue->lock);
    queue->waiters_head = queue->waiters_tail = NULL;
#ifdef WAIT_QUEUE_DEBUG
    debug_log("Initialized wait queue: [ %p, %s ]\n", queue, func);
#endif /* WAIT_QUEUE_DEBUG */
}

void __wait_queue_enqueue_task(struct wait_queue *queue, struct task *task, const char *func) {
    (void) func;
    assert(task->wait_queue_next == NULL);
    if (!queue->waiters_head) {
        queue->waiters_head = queue->waiters_tail = task;
    } else {
        queue->waiters_tail->wait_queue_next = task;
        queue->waiters_tail = task;
    }

#ifdef WAIT_QUEUE_DEBUG
    debug_log("enqueuing task: [ %p, %d:%d, %s ]\n", queue, task->process->pid, task->tid, func);
#endif /* WAIT_QUEUE_DEBUG */
}

bool __wake_up(struct wait_queue *queue, const char *func) {
    (void) func;
    struct task *to_wake = queue->waiters_head;
    if (to_wake) {
        assert(to_wake->sched_state != EXITING);
        to_wake->wait_interruptible = false;
        to_wake->sched_state = RUNNING_UNINTERRUPTIBLE;
#ifdef WAIT_QUEUE_DEBUG
        debug_log("waking up task: [ %p, %d:%d, %s ]\n", queue, to_wake->process->pid, to_wake->tid, func);
#endif /* WAIT_QUEUE_DEBUG */

        queue->waiters_head = to_wake->wait_queue_next;
        to_wake->wait_queue_next = NULL;

        if (queue->waiters_tail == to_wake) {
            queue->waiters_tail = NULL;
        }

        return true;
    }

    return false;
}

void __wake_up_n(struct wait_queue *queue, int n, const char *func) {
    for (int i = 0; i < n; i++) {
        if (!__wake_up(queue, func)) {
            break;
        }
    }
}

void wait_on_internal(struct wait_queue *queue, const char *func) {
    struct task *task = get_current_task();
    task->wait_queue_next = NULL;

    spin_lock(&queue->lock);
    __wait_queue_enqueue_task(queue, task, func);

    task->sched_state = WAITING;
    spin_unlock(&queue->lock);

    __kernel_yield();
}

void wake_up_internal(struct wait_queue *queue, const char *func) {
    spin_lock(&queue->lock);

    __wake_up(queue, func);

    spin_unlock(&queue->lock);
}

void wake_up_all_internal(struct wait_queue *queue, const char *func) {
    (void) func;
    spin_lock(&queue->lock);

    struct task *to_wake = queue->waiters_head;
    while (to_wake) {
        to_wake->sched_state = RUNNING_UNINTERRUPTIBLE;
#ifdef WAIT_QUEUE_DEBUG
        debug_log("waking up task: [ %p, %d:%d, %s ]\n", queue, to_wake->process->pid, to_wake->tid, func);
#endif /* WAIT_QUEUE_DEBUG */
        to_wake = to_wake->wait_queue_next;
    }

    queue->waiters_head = queue->waiters_tail = NULL;

    spin_unlock(&queue->lock);
}
