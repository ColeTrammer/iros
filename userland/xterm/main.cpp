#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>

#include "application.h"

volatile sig_atomic_t g_pid_dead = -1;

int main() {
    // Block SIGCHLD so that we are never going to be interrupted.
    // We will use pselect to wait for it when ready.
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &set, nullptr);

    // FIXME: There is some bug that makes it necessary to always ignore SIGTTOU,
    //        when only ignoring it for the tcsetpgrp calls should suffice
    signal(SIGTTOU, SIG_IGN);

    struct sigaction act;
    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    act.sa_handler = [](auto) {
        int status;
        pid_t ret = waitpid(-1, &status, WNOHANG);
        assert(ret != -1);
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            g_pid_dead = ret;
        }
    };
    if (sigaction(SIGCHLD, &act, nullptr) < 0) {
        perror("sigaction(SIGCHLD)");
        return 1;
    }

    Application application;
    return application.run();
}
