#include <signal.h>
#include <stddef.h>
#include <setjmp.h>
#include <stddef.h>

sighandler_t signal(int sig, sighandler_t handler) {
    (void) sig;
    (void) handler;

    return NULL;
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