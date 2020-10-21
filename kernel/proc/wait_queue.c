#include <limits.h>

#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/proc/task.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/sched/task_sched.h>

// #define WAIT_QUEUE_DEBUG

void init_wait_queue_internal(struct wait_queue *queue, const char *func) {
    (void) func;
    init_spinlock(&queue->lock);
    init_list(&queue->list);
#ifdef WAIT_QUEUE_DEBUG
    debug_log("Initialized wait queue: [ %p, %s ]\n", queue, func);
#endif /* WAIT_QUEUE_DEBUG */
}

void __wait_queue_enqueue_task(struct wait_queue *queue, struct task *task, const char *func) {
    (void) func;
    list_append(&queue->list, &task->wait_queue_list);

#ifdef WAIT_QUEUE_DEBUG
    debug_log("enqueuing task: [ %p, %d:%d, %s ]\n", queue, task->process->pid, task->tid, func);
#endif /* WAIT_QUEUE_DEBUG */
}

void __wake_up_n(struct wait_queue *queue, int n, const char *func) {
    (void) func;

    int i = 0;
    list_for_each_entry(&queue->list, task, struct task, wait_queue_list) {
        if (i >= n) {
            break;
        }

        if (task_unblock(task, 0)) {
            i++;
        }
    }
}

void __wait_queue_dequeue_task(struct wait_queue *queue, struct task *task, const char *func) {
    (void) func;
    (void) queue;
    list_remove(&task->wait_queue_list);
}

void wait_queue_dequeue_task(struct wait_queue *queue, struct task *task, const char *func) {
    spin_lock(&queue->lock);
    __wait_queue_dequeue_task(queue, task, func);
    spin_unlock(&queue->lock);
}

void wake_up_internal(struct wait_queue *queue, const char *func) {
    spin_lock(&queue->lock);

    __wake_up_n(queue, 1, func);

    spin_unlock(&queue->lock);
}

void wake_up_all_internal(struct wait_queue *queue, const char *func) {
    (void) func;
    spin_lock(&queue->lock);
    __wake_up_n(queue, INT_MAX, func);
    spin_unlock(&queue->lock);
}
