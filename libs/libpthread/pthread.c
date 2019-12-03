#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

pthread_t pthread_self(void);

int pthread_create(pthread_t *__restrict thread, const pthread_attr_t *__restrict attr, void *(*start_routine)(void *arg),
                   void *__restrict arg) {
    (void) attr;

    void *stack = mmap(NULL, 32 * 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_STACK, 0, 0);
    if (stack == MAP_FAILED) {
        return EAGAIN;
    }

    int ret = create_task((uintptr_t) start_routine, (uintptr_t) stack + 32 * 0x1000, arg, (uintptr_t) &pthread_exit);
    if (ret < 0) {
        return EAGAIN;
    }

    *thread = (pthread_t) ret;
    return 0;
}

int pthread_join(pthread_t thread, void **value_ptr) {
    (void) thread;
    (void) value_ptr;

    sleep(2);
    return 0;
}

__attribute__((__noreturn__)) void pthread_exit(void *value_ptr) {
    (void) value_ptr;
    exit_task();
}