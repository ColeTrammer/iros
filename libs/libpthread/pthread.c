#define __libc_internal

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/os_2.h>
#include <unistd.h>

static pthread_spinlock_t threads_lock = { 0 };

#if ARCH == X86_64

static struct thread_control_block *get_self() {
    struct thread_control_block *ret;
    asm("movq %%fs:0, %0" : "=r"(ret)::);
    return ret;
}

#endif /* ARCH == X86_64 */

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

    void *stack = mmap(NULL, 32 * 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_STACK, 0, 0);
    if (stack == MAP_FAILED) {
        return EAGAIN;
    }

    if (attr && *attr == -1) {
        return EINVAL;
    }

    struct thread_control_block *to_add = __allocate_thread_control_block();
    to_add->stack_start = stack;
    to_add->stack_len = 32 * 0x1000;

    if (attr) {
        memcpy(&to_add->attributes, attr, sizeof(pthread_attr_t));
    } else {
        pthread_attr_init(&to_add->attributes);
    }

    pthread_spin_lock(&threads_lock);
    __add_thread(to_add, __threads);
    if (__threads == NULL) {
        __threads = to_add;
    }

    int ret = create_task((uintptr_t) start_routine, (uintptr_t) stack + 32 * 0x1000, arg, (uintptr_t) &pthread_exit, &to_add->id, to_add);
    if (ret < 0) {
        return EAGAIN;
    }

    pthread_spin_unlock(&threads_lock);

    *thread = to_add->id;
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
        if (target->joining_thread != 0 || !(target->attributes == PTHREAD_CREATE_JOINABLE)) {
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
            munmap(target->stack_start, target->stack_len);
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
    return tgkill(0, thread, sig);
}

__attribute__((__noreturn__)) void pthread_exit(void *value_ptr) {
    struct thread_control_block *thread = get_self();
    if (thread->attributes & PTHREAD_CREATE_DETACHED) {
        pthread_spin_lock(&threads_lock);

        if (thread == __threads) {
            __threads = thread->next;
        }

        __remove_thread(thread);
        pthread_spin_unlock(&threads_lock);

        void *stack_start = thread->stack_start;
        size_t stack_len = thread->stack_len;

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
                     : "r"(stack_start), "r"(stack_len)
                     : "rdi", "rsi", "rdx", "rax", "memory");
#endif /* ARCH == X86_64 */
        __builtin_unreachable();
    }

    thread->exit_value = value_ptr;
    thread->has_exited = 1;

    exit_task();
    __builtin_unreachable();
}