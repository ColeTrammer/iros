#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

int main() {
    signal(SIGINT, [](int) {
        write(2, "\n", 1);
    });

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    assert(sigsuspend(&mask) == -1 && errno == EINTR);

    signal(SIGINT, SIG_DFL);
    sigsuspend(&mask);

    assert(false);
}
