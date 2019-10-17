#include <signal.h>
#include <stddef.h>
#include <setjmp.h>
#include <stddef.h>
#include <errno.h>

sighandler_t signal(int sig, sighandler_t handler) {
    (void) sig;
    (void) handler;

    return NULL;
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