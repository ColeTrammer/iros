#define __libc_internal

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/iros.h>

static void on_cancelation_request(int signum) {
    assert(signum == __PTHREAD_CANCEL_SIGNAL);

    // FIXME: this should only exit immediately if the
    //        signal was delivered before a system call
    //        was made, not after
    pthread_exit(PTHREAD_CANCELED);
}

void setup_cancelation_handler() {
    __cancelation_setup = 1;

    struct sigaction act;
    act.sa_handler = &on_cancelation_request;
    act.sa_flags = SA_RESTART;
    sigaction(__PTHREAD_CANCEL_SIGNAL, &act, NULL);
}

int pthread_cancel(pthread_t thread) {
    if (!__cancelation_setup) {
        // This is necessary if a single threaded program
        // attempts to cancel itself
        setup_cancelation_handler();
    }

    return tgkill(0, thread, __PTHREAD_CANCEL_SIGNAL);
}

int pthread_setcancelstate(int state, int *oldstate) {
    if (state != PTHREAD_CANCEL_DISABLE && state != PTHREAD_CANCEL_ENABLE) {
        return EINVAL;
    }

    struct thread_control_block *block = __get_self();
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
    } else {
        // NOTE: explicitly test any pending cancellations which were
        //       previously blocked.
        pthread_testcancel();
    }

    return 0;
}

int pthread_setcanceltype(int type, int *oldtype) {
    if (type != PTHREAD_CANCEL_ASYNCHRONOUS && type != PTHREAD_CANCEL_DEFERRED) {
        return EINVAL;
    }

    struct thread_control_block *block = __get_self();
    if (oldtype != NULL) {
        *oldtype = block->attributes.__flags & PTHREAD_CANCEL_ASYNCHRONOUS;
    }

    if ((block->attributes.__flags & PTHREAD_CANCEL_ASYNCHRONOUS) == type) {
        return 0;
    }

    block->attributes.__flags |= type;

    if (!(block->attributes.__flags & PTHREAD_CANCEL_DISABLE)) {
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
        sigfillset(&pending);
        sigdelset(&pending, __PTHREAD_CANCEL_SIGNAL);
        sigsuspend(&pending);
        assert(false);
    }
}
