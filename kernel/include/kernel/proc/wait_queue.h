#ifndef _KERNEL_PROC_WAIT_QUEUE_H
#define _KERNEL_PROC_WAIT_QUEUE_H 1

#include <stdbool.h>

#include <kernel/util/spinlock.h>

struct task;

struct wait_queue {
    spinlock_t lock;
    struct task *waiters_head;
    struct task *waiters_tail;
};

#define WAIT_QUEUE_INITIALIZER \
    { SPINLOCK_INITIALIZER, NULL, NULL }

void init_wait_queue(struct wait_queue *queue);

void __wait_queue_enqueue_task(struct wait_queue *queue, struct task *task);
void __wake_up_n(struct wait_queue *queue, int n);

void wait_on(struct wait_queue *queue);
void wake_up(struct wait_queue *queue);
void wake_up_all(struct wait_queue *queue);

static inline __attribute__((always_inline)) bool __wait_queue_is_empty(struct wait_queue *queue) {
    return queue->waiters_head == NULL;
}

#endif /* _KERNEL_PROC_WAIT_QUEUE_H */
