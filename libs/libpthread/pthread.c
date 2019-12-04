#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <search.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/os_2.h>
#include <unistd.h>

struct thread_control_block {
    struct thread_control_block *next;
    struct thread_control_block *prev;
    void *stack_start;
    size_t stack_len;
    pthread_t id;
    pthread_t joining_thread;
    int has_exited;
    void *exit_value;
};

static struct thread_control_block *threads;
static pthread_spinlock_t threads_lock = { 0 };

__attribute__((constructor)) __attribute__((used)) static void init_pthreads() {
    threads = calloc(1, sizeof(struct thread_control_block));
    threads->id = pthread_self();
}

pthread_t pthread_self(void) {
    return gettid();
}

int pthread_create(pthread_t *__restrict thread, const pthread_attr_t *__restrict attr, void *(*start_routine)(void *arg),
                   void *__restrict arg) {
    (void) attr;

    void *stack = mmap(NULL, 32 * 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_STACK, 0, 0);
    if (stack == MAP_FAILED) {
        return EAGAIN;
    }

    struct thread_control_block *to_add = calloc(1, sizeof(struct thread_control_block));
    to_add->stack_start = stack;
    to_add->stack_len = 32 * 0x1000;

    pthread_spin_lock(&threads_lock);
    insque(to_add, threads);
    if (threads == NULL) {
        threads = to_add;
    }

    int ret = create_task((uintptr_t) start_routine, (uintptr_t) stack + 32 * 0x1000, arg, (uintptr_t) &pthread_exit, &to_add->id);
    if (ret < 0) {
        return EAGAIN;
    }

    pthread_spin_unlock(&threads_lock);

    if (thread) {
        *thread = (pthread_t) to_add->id;
    }

    return 0;
}

int pthread_join(pthread_t id, void **value_ptr) {
    pthread_t self_id = pthread_self();
    if (id == self_id) {
        return EDEADLK;
    }

    int ret = ESRCH;

    pthread_spin_lock(&threads_lock);
    struct thread_control_block *thread = threads;
    struct thread_control_block *target = NULL;
    struct thread_control_block *self = NULL;
    while (thread && (!target || !self)) {
        if (thread->id == id) {
            target = thread;
        } else if (thread->id == self_id) {
            self = thread;
        }
        thread = thread->next;
    }

    assert(self);
    if (target) {
        if (target->joining_thread != 0) {
            ret = EINVAL;
        } else {
            target->joining_thread = self_id;
            pthread_spin_unlock(&threads_lock);

            while (!target->has_exited) {
                asm volatile("" : : : "memory");
            }

            pthread_spin_lock(&threads_lock);

            if (target == threads) {
                threads = target->next;
            }

            remque(target);
            munmap(target->stack_start, target->stack_len);
            free(target);

            if (value_ptr) {
                *value_ptr = target->exit_value;
            }

            ret = 0;
        }
    }

    pthread_spin_unlock(&threads_lock);
    return ret;
}

__attribute__((__noreturn__)) void pthread_exit(void *value_ptr) {
    pthread_t self = pthread_self();

    pthread_spin_lock(&threads_lock);
    struct thread_control_block *thread = threads;

    while (thread && thread->id != self) {
        thread = thread->next;
    }

    assert(thread);
    thread->exit_value = value_ptr;
    thread->has_exited = 1;

    pthread_spin_unlock(&threads_lock);

    exit_task();
}