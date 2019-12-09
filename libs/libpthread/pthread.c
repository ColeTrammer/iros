#define __libc_internal

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/os_2.h>
#include <sys/syscall.h>
#include <unistd.h>

static pthread_spinlock_t threads_lock = { 0 };

__thread struct __pthread_cleanup_handler *__cleanup_handlers = NULL;

static void pthread_exit_after_cleanup(void *value_ptr) __attribute__((__noreturn__));

struct thread_control_block *get_self() {
#if ARCH == X86_64
    struct thread_control_block *ret;
    asm("movq %%fs:0, %0" : "=r"(ret)::);
    return ret;
#endif /* ARCH == X86_64 */
}

static void __add_thread(struct thread_control_block *elem, struct thread_control_block *prev) {
    struct thread_control_block *to_add = elem;
    struct thread_control_block *ent = prev;

    if (prev == NULL) {
        to_add->next = NULL;
        to_add->prev = NULL;
        return;
    }

    // ent is prev
    to_add->next = ent->next;
    to_add->prev = ent;

    if (to_add->next) {
        to_add->next->prev = to_add;
    }
    ent->next = to_add;
}

static void __remove_thread(struct thread_control_block *block) {
    struct thread_control_block *to_remove = block;

    struct thread_control_block *prev = to_remove->prev;
    struct thread_control_block *next = to_remove->next;

    if (prev) {
        prev->next = to_remove->next;
    }

    if (next) {
        next->prev = to_remove->prev;
    }

    to_remove->next = NULL;
    to_remove->prev = NULL;
}

pthread_t pthread_self(void) {
    return get_self()->id;
}

int pthread_create(pthread_t *__restrict thread, const pthread_attr_t *__restrict attr, void *(*start_routine)(void *arg),
                   void *__restrict arg) {
    if (thread == NULL) {
        return EINVAL;
    }

    if (attr != NULL && attr->__flags == -1) {
        return EINVAL;
    }

    struct thread_control_block *to_add = __allocate_thread_control_block();

    if (attr != NULL) {
        memcpy(&to_add->attributes, attr, sizeof(pthread_attr_t));
    } else {
        pthread_attr_init(&to_add->attributes);
    }

    // Round everything up to PAGE_SIZE intervals
    to_add->attributes.__stack_len = ((to_add->attributes.__stack_len + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    to_add->attributes.__guard_size = ((to_add->attributes.__guard_size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

    uint8_t *stack = to_add->attributes.__stack_start != NULL ? to_add->attributes.__stack_start
                                                              : mmap(NULL, to_add->attributes.__stack_len + to_add->attributes.__guard_size,
                                                                     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_STACK, 0, 0);
    to_add->attributes.__stack_start = stack;
    if (stack == MAP_FAILED) {
        __free_thread_control_block(to_add);
        return EAGAIN;
    }

    if (to_add->attributes.__guard_size != 0 && !(to_add->attributes.__flags & __PTHREAD_MAUALLY_ALLOCATED_STACK)) {
        mprotect(stack + to_add->attributes.__stack_len, to_add->attributes.__guard_size, PROT_NONE);
    }

    pthread_spin_lock(&threads_lock);
    __add_thread(to_add, __threads);
    if (__threads == NULL) {
        __threads = to_add;
    }

    int ret = syscall(SC_CREATE_TASK, (uintptr_t) start_routine, (uintptr_t) stack + to_add->attributes.__stack_len, arg,
                      (uintptr_t) &pthread_exit_after_cleanup, &to_add->id, to_add);
    if (ret < 0) {
        return EAGAIN;
    }

    pthread_spin_unlock(&threads_lock);

    *thread = to_add->id;
    return 0;
}

int pthread_detach(pthread_t thread) {
    pthread_spin_lock(&threads_lock);

    if (thread == pthread_self()) {
        struct thread_control_block *block = get_self();
        if (block->joining_thread != 0 || block->attributes.__flags & PTHREAD_CREATE_DETACHED) {
            // Can't call on non joinable thread
            pthread_spin_unlock(&threads_lock);
            return EINVAL;
        }

        block->attributes.__flags |= PTHREAD_CREATE_DETACHED;
    } else {
        struct thread_control_block *block = __threads;
        while (block != NULL && block->id != thread) {
            block = block->next;
        }

        if (block == NULL) {
            pthread_spin_unlock(&threads_lock);
            return ESRCH;
        }

        if (block->joining_thread) {
            pthread_spin_unlock(&threads_lock);
            return EINVAL;
        }

        if (block->has_exited) {
            if (block == __threads) {
                __threads = block->next;
            }

            __remove_thread(block);
            if (!(block->attributes.__flags & __PTHREAD_MAUALLY_ALLOCATED_STACK)) {
                munmap(block->attributes.__stack_start, block->attributes.__stack_len + block->attributes.__guard_size);
            }
            __free_thread_control_block(block);
        } else {
            block->attributes.__flags |= PTHREAD_CREATE_DETACHED;
        }
    }

    pthread_spin_unlock(&threads_lock);
    return 0;
}

int pthread_equal(pthread_t t1, pthread_t t2) {
    return t1 == t2;
}

int pthread_join(pthread_t id, void **value_ptr) {
    pthread_t self_id = pthread_self();
    if (id == self_id) {
        return EDEADLK;
    }

    int ret = ESRCH;

    pthread_spin_lock(&threads_lock);
    struct thread_control_block *thread = __threads;
    struct thread_control_block *target = NULL;
    while (thread && !target) {
        if (thread->id == id) {
            target = thread;
        }
        thread = thread->next;
    }

    if (target) {
        if (target->joining_thread != 0 || (target->attributes.__flags & PTHREAD_CREATE_DETACHED)) {
            ret = EINVAL;
        } else {
            target->joining_thread = self_id;
            pthread_spin_unlock(&threads_lock);

            while (!target->has_exited) {
                asm volatile("" : : : "memory");
            }

            pthread_spin_lock(&threads_lock);

            if (target == __threads) {
                __threads = target->next;
            }

            __remove_thread(target);
            if (!(target->attributes.__flags & __PTHREAD_MAUALLY_ALLOCATED_STACK)) {
                munmap(target->attributes.__stack_start, target->attributes.__stack_len + target->attributes.__guard_size);
            }
            __free_thread_control_block(target);

            if (value_ptr) {
                *value_ptr = target->exit_value;
            }

            ret = 0;
        }
    }

    pthread_spin_unlock(&threads_lock);
    return ret;
}

int pthread_kill(pthread_t thread, int sig) {
    return syscall(SC_TGKILL, 0, thread, sig);
}

int pthread_sigmask(int how, const sigset_t *__restrict set, sigset_t *__restrict old) {
    return sigprocmask(how, set, old);
}

__attribute__((__noreturn__)) void pthread_exit(void *value_ptr) {
    while (__cleanup_handlers != NULL) {
        (__cleanup_handlers->__func)(__cleanup_handlers->__arg);
        __cleanup_handlers = __cleanup_handlers->__next;
    }

    pthread_exit_after_cleanup(value_ptr);
}

__attribute__((__noreturn__)) static void pthread_exit_after_cleanup(void *value_ptr) {
    struct thread_control_block *thread = get_self();

    pthread_specific_run_destructors(thread);

    if (thread->attributes.__flags & PTHREAD_CREATE_DETACHED) {
        pthread_spin_lock(&threads_lock);

        if (thread == __threads) {
            __threads = thread->next;
        }

        __remove_thread(thread);
        pthread_spin_unlock(&threads_lock);

        // This means there all threads are dead, just exit and let the kernel
        // clean everything up. This is safe to check after the lock is released
        // since at this point, only this thread is alive anyway.
        if (__threads == NULL) {
            exit(0);
        }

        // Only deallocate stacks we created ourselves
        if (thread->attributes.__flags & __PTHREAD_MAUALLY_ALLOCATED_STACK) {
            syscall(SC_EXIT_TASK);
            __builtin_unreachable();
        }

        void *stack_start = thread->attributes.__stack_start;
        size_t stack_len = thread->attributes.__stack_len;
        size_t guard_len = thread->attributes.__guard_size;

        __free_thread_control_block(thread);
#if ARCH == X86_64
        // Call munmap and task_exit
        asm volatile("movq $46, %%rdi\n"
                     "movq %0, %%rsi\n"
                     "movq %1, %%rdx\n"
                     "int $0x80\n"
                     "movq $56, %%rdi\n"
                     "int $0x80"
                     :
                     : "r"(stack_start), "r"(stack_len + guard_len)
                     : "rdi", "rsi", "rdx", "rax", "memory");
#endif /* ARCH == X86_64 */
        __builtin_unreachable();
    }

    // This means there all threads are dead, just exit and let the kernel
    // clean everything up. This is safe to check without the lock
    // since at this point, only this thread is alive anyway. If another
    // thread did exist and was modifying this list, it would make this
    // condition impossible.
    if (thread->next == NULL && __threads == thread) {
        exit(0);
    }

    thread->exit_value = value_ptr;
    thread->has_exited = 1;

    syscall(SC_EXIT_TASK);
    __builtin_unreachable();
}

int pthread_getconcurrency(void) {
    return get_self()->concurrency;
}

int pthread_getschedparam(pthread_t thread, int *__restrict policy, struct sched_param *__restrict param) {
    if (policy == NULL || param == NULL) {
        return EINVAL;
    }

    struct thread_control_block *block = NULL;
    if (thread == pthread_self()) {
        block = get_self();
    }

    pthread_spin_lock(&threads_lock);
    struct thread_control_block *to_search = block ? block : __threads;
    while (to_search != NULL) {
        if (to_search->id == thread) {
            block = to_search;
            break;
        }

        to_search = to_search->next;
    }

    if (to_search == NULL) {
        pthread_spin_unlock(&threads_lock);
        return ESRCH;
    }

    *policy = block->attributes.__flags & __SCHED_MASK;
    memcpy(param, &block->attributes.__sched_param, sizeof(struct sched_param));

    pthread_spin_unlock(&threads_lock);
    return 0;
}

int pthread_setconcurrency(int new_level) {
    get_self()->concurrency = new_level;
    return 0;
}

int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param) {
    if (param == NULL || (policy != SCHED_FIFO && policy != SCHED_RR && policy != SCHED_OTHER && policy != SCHED_SPORADIC)) {
        return EINVAL;
    }

    struct thread_control_block *block = NULL;
    if (thread == pthread_self()) {
        block = get_self();
    }

    pthread_spin_lock(&threads_lock);
    struct thread_control_block *to_search = block ? block : __threads;
    while (to_search != NULL) {
        if (to_search->id == thread) {
            block = to_search;
            break;
        }

        to_search = to_search->next;
    }

    if (to_search == NULL) {
        pthread_spin_unlock(&threads_lock);
        return ESRCH;
    }

    // FIXME: this should do a system call that actually updates these values
    block->attributes.__flags |= policy;
    memcpy(&block->attributes.__sched_param, param, sizeof(struct sched_param));

    pthread_spin_unlock(&threads_lock);
    return 0;
}

int pthread_setschedprio(pthread_t thread, int prio) {
    struct thread_control_block *block = NULL;
    if (thread == pthread_self()) {
        block = get_self();
    }

    pthread_spin_lock(&threads_lock);
    struct thread_control_block *to_search = block ? block : __threads;
    while (to_search != NULL) {
        if (to_search->id == thread) {
            block = to_search;
            break;
        }

        to_search = to_search->next;
    }

    if (to_search == NULL) {
        pthread_spin_unlock(&threads_lock);
        return ESRCH;
    }

    // FIXME: this should do a system call that actually updates these values
    block->attributes.__sched_param.sched_priority = prio;

    pthread_spin_unlock(&threads_lock);
    return 0;
}