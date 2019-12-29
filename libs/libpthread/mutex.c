#define __libc_internal

#include <alloca.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
#include <sys/os_2.h>

int pthread_mutex_consistent(pthread_mutex_t *mutex) {
    if (!mutex || !(mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) || !(mutex->__attr.__flags & __PTHREAD_MUTEX_INCONSISTENT)) {
        return EINVAL;
    }

    // Maybe should be atomic since apparently there's no requirement that the mutex is locked
    mutex->__attr.__flags &= ~(__PTHREAD_MUTEX_INCONSISTENT);
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *__restrict mutex, const pthread_mutexattr_t *__restrict mutexattr) {
    if (mutex == NULL) {
        return EINVAL;
    }

    mutex->__lock = 0;
    mutex->__count_if_recursive = 0;
    if (mutexattr != NULL) {
        memcpy(&mutex->__attr, mutexattr, sizeof(pthread_mutexattr_t));
    } else {
        pthread_mutexattr_init(&mutex->__attr);
    }
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    int expected = 0;
    pthread_t tid = pthread_self();

    struct __locked_robust_mutex_node *robust_mutex_node = NULL;
    struct thread_control_block *self = NULL;
    if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
        self = get_self();

        robust_mutex_node = alloca(sizeof(struct __locked_robust_mutex_node));
        robust_mutex_node->__in_progress_flags = ROBUST_MUTEX_IS_VALID_IF_VALUE;
        robust_mutex_node->__in_progress_value = tid;
        robust_mutex_node->__prev = NULL;
        robust_mutex_node->__next = self->locked_robust_mutex_node_list_head;
        robust_mutex_node->__protected = &mutex->__lock;

        self->locked_robust_mutex_node_list_head = robust_mutex_node;
    }

    while (!atomic_compare_exchange_strong(&mutex->__lock, &expected, tid)) {
        // Failed to aquire the lock
        if ((mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) && (mutex->__attr.__flags & __PTHREAD_MUTEX_UNRECOVERABLE)) {
            self->locked_robust_mutex_node_list_head = robust_mutex_node->__next;
            return ENOTRECOVERABLE;
        }

        if ((mutex->__attr.__flags & PTHREAD_MUTEX_ERRORCHECK) && (expected == tid)) {
            if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
                self->locked_robust_mutex_node_list_head = robust_mutex_node->__next;
            }
            return EDEADLK;
        } else if ((mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) && (expected == tid)) {
            // Don't need any locks since only one thread owns this
            mutex->__count_if_recursive++;

            if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
                self->locked_robust_mutex_node_list_head = robust_mutex_node->__next;
            }
            return 0;
        }

        if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST && mutex->__lock == MUTEX_OWNER_DIED) {
            // This effectively means the thread that owns the lock is gone, so now try to lock it. Else continue.
            if (atomic_compare_exchange_strong(&mutex->__lock, &expected, tid)) {
                // Now we have the lock and know the owner died.
                mutex->__attr.__flags |= __PTHREAD_MUTEX_INCONSISTENT;

                if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
                    robust_mutex_node->__in_progress_flags = 0;
                    memcpy(&mutex->__locked_robust_mutex_node, robust_mutex_node, sizeof(struct __locked_robust_mutex_node));
                    self->locked_robust_mutex_node_list_head = &mutex->__locked_robust_mutex_node;
                }

                return EOWNERDEAD;
            }
        }

        os_mutex(&mutex->__lock, MUTEX_AQUIRE, expected, tid, 0, NULL);
        expected = 0;
    }

    if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
        robust_mutex_node->__in_progress_flags = 0;
        memcpy(&mutex->__locked_robust_mutex_node, robust_mutex_node, sizeof(struct __locked_robust_mutex_node));
        self->locked_robust_mutex_node_list_head = &mutex->__locked_robust_mutex_node;
    }

    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    int expected = 0;
    pthread_t tid = pthread_self();

    struct __locked_robust_mutex_node *robust_mutex_node = NULL;
    struct thread_control_block *self = NULL;
    if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
        self = get_self();

        robust_mutex_node = alloca(sizeof(struct __locked_robust_mutex_node));
        robust_mutex_node->__in_progress_flags = ROBUST_MUTEX_IS_VALID_IF_VALUE;
        robust_mutex_node->__in_progress_value = tid;
        robust_mutex_node->__prev = NULL;
        robust_mutex_node->__next = self->locked_robust_mutex_node_list_head;
        robust_mutex_node->__protected = &mutex->__lock;

        self->locked_robust_mutex_node_list_head = robust_mutex_node;
    }

    if (!atomic_compare_exchange_strong(&mutex->__lock, &expected, tid)) {
        if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
            self->locked_robust_mutex_node_list_head = robust_mutex_node->__next;
        }

        if ((mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) && (mutex->__attr.__flags & __PTHREAD_MUTEX_UNRECOVERABLE)) {
            return ENOTRECOVERABLE;
        }

        if ((mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) && (expected == tid)) {
            mutex->__count_if_recursive++;
            return 0;
        }

        // Failed to aquire the lock
        return EBUSY;
    }

    if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
        robust_mutex_node->__in_progress_flags = 0;
        memcpy(&mutex->__locked_robust_mutex_node, robust_mutex_node, sizeof(struct __locked_robust_mutex_node));
        self->locked_robust_mutex_node_list_head = &mutex->__locked_robust_mutex_node;
    }

    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    pthread_t tid = pthread_self();
    if ((mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) || (mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) ||
        (mutex->__attr.__flags & PTHREAD_MUTEX_ERRORCHECK)) {
        if (mutex->__lock != (unsigned int) tid) {
            return EPERM;
        }
    }

    if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
        if (mutex->__attr.__flags & __PTHREAD_MUTEX_UNRECOVERABLE) {
            return 0;
        }

        if (mutex->__attr.__flags & __PTHREAD_MUTEX_INCONSISTENT) {
            mutex->__attr.__flags |= __PTHREAD_MUTEX_UNRECOVERABLE;

            // Wake all threads waiting so they'll return ENOTRECOVERABLE (store 1 so nobody can ever)
            // lock it again, since tid 1 is reserved for the kernel
            return os_mutex(&mutex->__lock, MUTEX_WAKE_AND_SET, tid, 1, INT_MAX, NULL);
        }
    }

    if ((mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) && --mutex->__count_if_recursive != 0) {
        return 0;
    }

    if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
        mutex->__locked_robust_mutex_node.__in_progress_value = tid;
        mutex->__locked_robust_mutex_node.__in_progress_flags |= ROBUST_MUTEX_IS_VALID_IF_VALUE;
    }

    int ret = os_mutex(&mutex->__lock, MUTEX_WAKE_AND_SET, tid, 0, 1, NULL);

    if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
        if (!mutex->__locked_robust_mutex_node.__prev) {
            get_self()->locked_robust_mutex_node_list_head = mutex->__locked_robust_mutex_node.__next;
        } else {
            mutex->__locked_robust_mutex_node.__prev = mutex->__locked_robust_mutex_node.__next;
        }
    }

    return ret;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    if (mutex == NULL || mutex->__lock == -1U) {
        return EINVAL;
    }

    mutex->__lock = -1;
    return 0;
}