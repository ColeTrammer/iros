#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    auto on_int = [](int sig) {
        assert(sig == SIGINT);
        // ucontext_t* context = reinterpret_cast<ucontext_t*>(_context);

        // fprintf(stderr, "rax: %#.16lX\n", context->uc_mcontext.__cpu_state.rax);
    };

    struct sigaction act;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);
    act.sa_handler = on_int;
    sigaction(SIGINT, &act, nullptr);

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);
    sigsuspend(&set);

    write(2, "returned\n", 9);
    return 0;
}