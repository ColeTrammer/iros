#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    char *cat1_args[] = { "ls", NULL };
    char *cat2_args[] = { "cat", NULL };

    int fds[2];
    pipe(fds);

    pid_t ret = fork();
    if (ret < 0) {
        perror("pipe_test");
        return 1;
    }

    /* Child */
    else if (ret == 0) {
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);

        execvp(cat1_args[0], cat1_args);
    }

    /* Parent */
    else {
        close(fds[1]);
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);

        int status;
        do {
            waitpid(ret, &status, WUNTRACED);
        } while (!WIFEXITED(status));
        execvp(cat2_args[0], cat2_args);
    }

    return 1;
}