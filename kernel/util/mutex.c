#include <stdatomic.h>

#include <kernel/sched/task_sched.h>
#include <kernel/util/mutex.h>

void init_mutex(mutex_t *mutex) {
    init_wait_queue(&mutex->queue);
    mutex->lock = 0;
}

static void __mutex_lock(mutex_t *mutex, struct task *task) {
    for (;;) {
        if (mutex->lock == 0) {
            mutex->lock = 1;
            return;
        }

        __wait_queue_enqueue_task(&mutex->queue, task);
        spin_unlock(&mutex->queue.lock);
        __kernel_yield();
    }
}

static bool __mutex_trylock(mutex_t *mutex) {
    if (mutex->lock == 0) {
        mutex->lock = 1;
        return true;
    }
    return false;
}

static void __mutex_unlock(mutex_t *mutex) {
    assert(mutex->lock == 1);
    __wake_up_n(&mutex->queue, 1);
}

extern struct task *current_task;

void mutex_lock(mutex_t *mutex) {
    spin_lock(&mutex->queue.lock);
    __mutex_lock(mutex, current_task);
    spin_unlock(&mutex->queue.lock);
}

bool mutex_trylock(mutex_t *mutex) {
    spin_lock(&mutex->queue.lock);
    bool ret = __mutex_trylock(mutex);
    spin_unlock(&mutex->queue.lock);
    return ret;
}

void mutex_unlock(mutex_t *mutex) {
    spin_lock(&mutex->queue.lock);
    __mutex_unlock(mutex);
    spin_unlock(&mutex->queue.lock);
}
