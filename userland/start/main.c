#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
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
        }
        _exit(execvp(argv[0], argv));
    } else if (pid == -1) {
        perror("fork");
        _exit(1);
    }

    setpgid(pid, pid);
}

int main() {
    char *nslookup_args[] = { "/bin/nslookup", "-s", NULL };

    spawn_process(nslookup_args, false);
#ifdef KERNEL_NO_GRAPHICS
    char *xterm_args[] = { "/bin/xterm", NULL };

    spawn_process(xterm_args, false);
#else
    char *window_server_args[] = { "/bin/window_server", NULL };

    spawn_process(window_server_args, false);

    sleep(1);

    char *window_server_test_args[] = { "/initrd/window_server_test", NULL };

    spawn_process(window_server_test_args, false);
#endif /* KERNEL_NO_GRAPHICS */

    for (;;) {
        sleep(100);
        waitpid(-1, NULL, WNOHANG);
    }

    assert(false);
    return 0;
}