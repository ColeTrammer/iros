#ifndef _KERNEL_PROC_WAIT_QUEUE_H
#define _KERNEL_PROC_WAIT_QUEUE_H 1

#include <kernel/util/list.h>
#include <kernel/util/spinlock.h>

struct task;

struct wait_queue {
    spinlock_t lock;
    struct list_node list;
};

#define WAIT_QUEUE_INITIALIZER(wq) \
    { SPINLOCK_INITIALIZER, INIT_LIST((wq).list) }

void __wait_queue_dequeue_task(struct wait_queue *queue, struct task *task, const char *func);
void __wait_queue_enqueue_task(struct wait_queue *queue, struct task *task, const char *func);
void __wake_up_n(struct wait_queue *queue, int n, const char *func);

void wait_queue_dequeue_task(struct wait_queue *queue, struct task *task, const char *func);
void init_wait_queue_internal(struct wait_queue *queue, const char *func);
void wake_up_internal(struct wait_queue *queue, const char *func);
void wake_up_all_internal(struct wait_queue *queue, const char *func);

#define init_wait_queue(w) init_wait_queue_internal(w, __func__)
#define wake_up(w)         wake_up_internal(w, __func__)
#define wake_up_all(w)     wake_up_all_internal(w, __func__)

static inline __attribute__((always_inline)) bool __wait_queue_is_empty(struct wait_queue *queue) {
    return list_is_empty(&queue->list);
}

#endif /* _KERNEL_PROC_WAIT_QUEUE_H */
