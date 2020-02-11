#include <setjmp.h>
#include <signal.h>

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