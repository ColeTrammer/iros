#include <signal.h>
#include <stddef.h>
#include <setjmp.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>

int raise(int signum) {
    // Should do something else for multithreaded programs
    return kill(getpid(), signum);
}

sighandler_t signal(int signum, sighandler_t handler) {
    struct sigaction act;
    struct sigaction old;

    act.sa_handler = handler;
    act.sa_flags = SA_RESTART; // glibc does this
    sigemptyset(&act.sa_mask);

    if (sigaction(signum, &act, &old)) {
        return SIG_ERR;
    }

    return old.sa_handler;
}

int sigemptyset(sigset_t *set) {
    *set = 0;
    return 0;
}

int sigfillset(sigset_t *set) {
    *set = ~0;
    return 0;
}

int sigaddset(sigset_t *set, int signum) {
    if (signum < 1 || signum > _NSIG) {
        errno = EINVAL;
        return -1;
    }
    *set |= (1U << (signum - 1));
    return 0;
}

int sigdelset(sigset_t *set, int signum) {
    if (signum < 1 || signum > _NSIG) {
        errno = EINVAL;
        return -1;
    }
    *set &= ~(1U << (signum - 1));
    return 0;
}

int sigismember(const sigset_t *set, int signum) {
    if (signum < 1 || signum > _NSIG) {
        errno = EINVAL;
        return -1;
    }
    return *set & (1U << (signum - 1)) ? 1 : 0;
}

// Called from sigsetjmp assembly function after registers are set
int __sigsetjmp(sigjmp_buf env, int val) {
    if (val) {
        env->is_mask_saved = 1;
        sigprocmask(0, NULL, &env->mask);
    } else {
         env->is_mask_saved = 0;
    }
    return 0;
}

__attribute__((noreturn))
void siglongjmp(sigjmp_buf env, int val) {
    if (env->is_mask_saved) {
        sigprocmask(SIG_SETMASK, &env->mask, NULL);
    }

    longjmp(env, val);
}