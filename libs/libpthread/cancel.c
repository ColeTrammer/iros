#define __libc_internal

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/syscall.h>

int pthread_cancel(pthread_t thread) {
    return syscall(SC_TGKILL, 0, thread, __PTHREAD_CANCEL_SIGNAL);
}

int pthread_setcancelstate(int state, int *oldstate) {
    if (state != PTHREAD_CANCEL_DISABLE && state != PTHREAD_CANCEL_ENABLE) {
        return EINVAL;
    }

    struct thread_control_block *block = get_self();
    if (oldstate != NULL) {
        *oldstate = block->attributes.__flags & PTHREAD_CANCEL_DISABLE;
    }

    if ((block->attributes.__flags & PTHREAD_CANCEL_DISABLE) == state) {
        return 0;
    }

    block->attributes.__flags |= state;

    if (block->attributes.__flags & PTHREAD_CANCEL_ASYNCHRONOUS) {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, __PTHREAD_CANCEL_SIGNAL);
        if (block->attributes.__flags & PTHREAD_CANCEL_DISABLE) {
            sigprocmask(SIG_BLOCK, &set, NULL);
        } else {
            sigprocmask(SIG_UNBLOCK, &set, NULL);
        }
    }

    return 0;
}

int pthread_setcanceltype(int type, int *oldtype) {
    if (type != PTHREAD_CANCEL_ASYNCHRONOUS && type != PTHREAD_CANCEL_DEFERRED) {
        return EINVAL;
    }

    struct thread_control_block *block = get_self();
    if (oldtype != NULL) {
        *oldtype = block->attributes.__flags & PTHREAD_CANCEL_ASYNCHRONOUS;
    }

    if ((block->attributes.__flags & PTHREAD_CANCEL_ASYNCHRONOUS) == type) {
        return 0;
    }

    block->attributes.__flags |= type;

    if (block->attributes.__flags & PTHREAD_CANCEL_ENABLE) {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, __PTHREAD_CANCEL_SIGNAL);
        if (block->attributes.__flags & PTHREAD_CANCEL_ASYNCHRONOUS) {
            sigprocmask(SIG_UNBLOCK, &set, NULL);
        } else {
            sigprocmask(SIG_BLOCK, &set, NULL);
        }
    }

    return 0;
}

void pthread_testcancel(void) {
    sigset_t pending;
    sigpending(&pending);
    if (sigismember(&pending, __PTHREAD_CANCEL_SIGNAL)) {
        sigemptyset(&pending);
        sigaddset(&pending, __PTHREAD_CANCEL_SIGNAL);
        sigsuspend(&pending);
        assert(false);
    }
}