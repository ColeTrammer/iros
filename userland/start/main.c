#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

void spawn_process(char **argv, bool redirect) {
    int pid = fork();
    if (pid == 0) {
        setpgid(0, 0);
        if (redirect) {
            int dev_null = open("/dev/null", O_RDWR, 0);
            dup2(dev_null, 0);
            dup2(dev_null, 1);
            dup2(dev_null, 2);
            close(dev_null);
        } else {
            signal(SIGTTOU, SIG_IGN);
            signal(SIGTTIN, SIG_IGN);

            tcsetpgrp(STDOUT_FILENO, getpgid(0));

            signal(SIGTTOU, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
        }
        _exit(execvp(argv[0], argv));
    } else if (pid == -1) {
        perror("fork");
        _exit(1);
    }

    setpgid(pid, pid);
}

int main() {
    char *nslookup_args[] = {
        "nslookup", "-s", NULL
    };

    char *sh_args[] = {
        "sh", NULL
    };

    spawn_process(nslookup_args, true);
    spawn_process(sh_args, false);

    for (;;) {
        sleep(100);
    }

    assert(false);
    return 0;
}