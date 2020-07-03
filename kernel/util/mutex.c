#include <stdatomic.h>

#include <kernel/sched/task_sched.h>
#include <kernel/util/mutex.h>

// #define MUTEX_DEBUG

void init_mutex_internal(mutex_t *mutex, const char *func) {
    (void) func;
    init_wait_queue(&mutex->queue);
    mutex->lock = 0;
#ifdef MUTEX_DEBUG
    debug_log("Initialized mutex: [ %p, %s ]\n", mutex, func);
#endif /* MUTEX_DEBUG */
}

static void __mutex_lock(mutex_t *mutex, struct task *task, const char *func) {
    (void) func;
    for (;;) {
        if (mutex->lock == 0) {
            mutex->lock = 1;
#ifdef MUTEX_DEBUG
            debug_log("Aquired mutex: [ %p, %s ]\n", mutex, func);
#endif /* MUTEX_DEBUG */
            return;
        }

        __wait_queue_enqueue_task(&mutex->queue, task);
        task->sched_state = WAITING;
#ifdef MUTEX_DEBUG
        debug_log("Failed to aquire mutex: [ %p, %s ]\n", mutex, func);
#endif /* MUTEX_DEBUG */
        spin_unlock(&mutex->queue.lock);

        __kernel_yield();

        spin_lock(&mutex->queue.lock);
    }
}

static bool __mutex_trylock(mutex_t *mutex, const char *func) {
    (void) func;
    if (mutex->lock == 0) {
        mutex->lock = 1;
#ifdef MUTEX_DEBUG
        debug_log("Aquired mutex: [ %p, %s ]\n", mutex, func);
#endif /* MUTEX_DEBUG */
        return true;
    }

#ifdef MUTEX_DEBUG
    debug_log("Failed to aquire mutex: [ %p, %s ]\n", mutex, func);
#endif /* MUTEX_DEBUG */
    return false;
}

static void __mutex_unlock(mutex_t *mutex, const char *func) {
    (void) func;
    assert(mutex->lock == 1);
    __wake_up_n(&mutex->queue, 1);
    mutex->lock = 0;
#ifdef MUTEX_DEBUG
    debug_log("Unlocked mutex: [ %p, %s ]\n", mutex, func);
#endif /* MUTEX_DEBUG */
}

extern struct task *current_task;

void mutex_lock_internal(mutex_t *mutex, const char *func) {
    spin_lock(&mutex->queue.lock);
    __mutex_lock(mutex, current_task, func);
    spin_unlock(&mutex->queue.lock);
}

bool mutex_trylock_internal(mutex_t *mutex, const char *func) {
    spin_lock(&mutex->queue.lock);
    bool ret = __mutex_trylock(mutex, func);
    spin_unlock(&mutex->queue.lock);
    return ret;
}

void mutex_unlock_internal(mutex_t *mutex, const char *func) {
    spin_lock(&mutex->queue.lock);
    __mutex_unlock(mutex, func);
    spin_unlock(&mutex->queue.lock);
}
