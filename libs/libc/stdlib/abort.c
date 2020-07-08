#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __is_libk
#include <kernel/hal/processor.h>
#endif /* __is_libk */

__attribute__((__noreturn__)) void abort() {
#ifdef __is_libk
    debug_log("\npanic\n");

    broadcast_panic();

    asm volatile("cli");
    while (1)
        ;
    __builtin_unreachable();
#else
    // Unblock SIGABRT, and block everything else
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGABRT);
    sigprocmask(SIG_SETMASK, &set, NULL);

    raise(SIGABRT);

    // If this didn't abort the process, we need to reset signal handling and try again
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = SIG_DFL;
    sigaction(SIGABRT, &act, NULL);

    raise(SIGABRT);

    // If this still didn't work, loop forever
    while (1)
        ;
    __builtin_unreachable();
#endif /* __is_libk */
}
