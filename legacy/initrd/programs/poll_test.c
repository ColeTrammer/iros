#include <assert.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    char c = 'a';

    int fds[2];
    if (pipe(fds)) {
        perror("poll_test: pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        sleep(1);
        if (write(fds[1], &c, 1) != 1) {
            perror("poll_test: write");
            return 1;
        }
        printf("Wrote to pipe\n");
        return 0;
    } else if (pid < 0) {
        perror("poll_test: fork");
        return 1;
    }

    struct pollfd poll_fds[1] = { { .fd = fds[0], .events = POLLIN, .revents = 0 } };
    if (poll(poll_fds, 1, -1) < 0) {
        perror("poll_test: poll");
        return 1;
    }

    assert(poll_fds[0].revents & POLLIN);
    if (read(fds[0], &c, 1) != 1) {
        perror("poll_test: read");
        return 1;
    }
    return 0;
}
