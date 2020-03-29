#include <kernel/proc/task.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/sched/task_sched.h>

void init_wait_queue(struct wait_queue *queue) {
    init_spinlock(&queue->lock);
    queue->waiters_head = queue->waiters_tail = NULL;
}

void __wait_queue_enqueue_task(struct wait_queue *queue, struct task *task) {
    if (!queue->waiters_head) {
        queue->waiters_head = queue->waiters_tail = task;
    } else {
        queue->waiters_tail->wait_queue_next = task;
        queue->waiters_tail = task;
    }
}

bool __wake_up(struct wait_queue *queue) {
    struct task *to_wake = queue->waiters_head;
    if (to_wake) {
        to_wake->sched_state = RUNNING_UNINTERRUPTIBLE;
        queue->waiters_head = to_wake->wait_queue_next;

        if (queue->waiters_tail == to_wake) {
            queue->waiters_tail = NULL;
        }

        return true;
    }

    return false;
}

void __wake_up_n(struct wait_queue *queue, int n) {
    for (int i = 0; i < n; i++) {
        if (!__wake_up(queue)) {
            break;
        }
    }
}

void wait_on(struct wait_queue *queue) {
    struct task *task = get_current_task();
    task->wait_queue_next = NULL;

    spin_lock(&queue->lock);
    __wait_queue_enqueue_task(queue, task);

    task->sched_state = WAITING;
    spin_unlock(&queue->lock);

    __kernel_yield();
}

void wake_up(struct wait_queue *queue) {
    spin_lock(&queue->lock);

    __wake_up(queue);

    spin_unlock(&queue->lock);
}

void wake_up_all(struct wait_queue *queue) {
    spin_lock(&queue->lock);

    struct task *to_wake = queue->waiters_head;
    while (to_wake) {
        to_wake->sched_state = RUNNING_UNINTERRUPTIBLE;
        to_wake = to_wake->wait_queue_next;
    }

    queue->waiters_head = queue->waiters_tail = NULL;

    spin_unlock(&queue->lock);
}