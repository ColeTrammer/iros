#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-w]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    signal(SIGALRM, [](int) {
        write(STDOUT_FILENO, "Hello from alarm\n", 17);
    });

    bool do_wait = false;

    int opt;
    while ((opt = getopt(argc, argv, ":w")) != -1) {
        switch (opt) {
            case 'w':
                do_wait = true;
                break;
        }
    }

    if (!do_wait) {
        alarm(2);
        write(STDOUT_FILENO, "Registered\n", 11);
        execl("/proc/self/exe", argv[0], "-w", NULL);
        perror("alarm_test: execl");
        return 127;
    } else {
        sigset_t set;
        sigfillset(&set);
        sigdelset(&set, SIGALRM);

        write(STDOUT_FILENO, "Waiting\n", 8);
        assert(sigsuspend(&set) == -1);
        assert(errno = EINTR);
        write(STDOUT_FILENO, "After\n", 6);

        // Test alarm cancelling
        alarm(1);
        alarm(0);

        write(STDOUT_FILENO, "Cancelled\n", 10);
        sleep(2);
        return 0;
    }
}
