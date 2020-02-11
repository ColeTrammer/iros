#include <setjmp.h>
#include <signal.h>

__attribute__((noreturn)) void siglongjmp(sigjmp_buf env, int val) {
    if (env->is_mask_saved) {
        sigprocmask(SIG_SETMASK, &env->mask, NULL);
    }

    longjmp(env, val);
}