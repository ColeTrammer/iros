#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

int main() {
#ifdef __iros__
    sigset_t set;
    sigemptyset(&set);
    printf("Empty set: %#.16" PRIX64 "\n", set);

    for (int test_sig = 1; test_sig < _NSIG; test_sig++) {
        if (sigaddset(&set, test_sig)) {
            perror("sigaddset");
            return 1;
        }

        printf("After adding `%s' (%d): %#.16" PRIX64 "\n", strsignal(test_sig), test_sig, set);

        assert(sigismember(&set, test_sig));

        if (sigdelset(&set, test_sig)) {
            perror("sigdelset");
            return 1;
        }

        assert(!sigismember(&set, test_sig));
    }

    sigfillset(&set);
    printf("Full set: %#.16" PRIX64 "\n", set);

#endif /* __iros__ */
    return 0;
}
