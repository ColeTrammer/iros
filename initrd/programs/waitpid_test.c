#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static void on_sigchld(int signo) {
    assert(signo == SIGCHLD);
    printf("sigchld\n");
}

int main() {
    struct sigaction act = { 0 };
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = on_sigchld;
    if (sigaction(SIGCHLD, &act, NULL)) {
        perror("sigaction");
        return 1;
    }

    // sigset_t set;
    // sigemptyset(&set);
    // sigaddset(&set, SIGCHLD);
    // sigprocmask(SIG_BLOCK, &set, NULL);

    pid_t pid = fork();
    if (pid == 0) {
        sleep(1);
        printf("child\n");
        _exit(0);
    } else if (pid < 0) {
        perror("fork");
        return 1;
    }

    int status;
    pid_t ret = waitpid(pid, &status, 0);
    if (ret < 0) {
        perror("waitpid");
        return 1;
    }

    printf("did waitpid\n");

    // sigprocmask(SIG_UNBLOCK, &set, NULL);

    assert(ret == pid);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
    return 0;
}
