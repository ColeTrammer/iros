#include <errno.h>
#include <pthread.h>
#include <unistd.h>

pthread_t pthread_self(void);

int pthread_create(pthread_t *__restrict thread, const pthread_attr_t *__restrict attr, void *(*start_routine)(void *arg),
                   void *__restrict arg) {
    (void) thread;
    (void) attr;
    (void) start_routine;
    (void) arg;

    return -EAGAIN;
}

int pthread_join(pthread_t thread, void **value_ptr) {
    (void) thread;
    (void) value_ptr;

    return 0;
}

__attribute__((__noreturn__)) void pthread_exit(void *value_ptr) {
    (void) value_ptr;
    exit_task();
}