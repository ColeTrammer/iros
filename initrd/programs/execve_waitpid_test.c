#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-w]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    bool do_wait = false;

    int opt;
    while ((opt = getopt(argc, argv, ":w")) != -1) {
        switch (opt) {
            case 'w':
                do_wait = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (do_wait) {
        while (wait(NULL) != -1) {
            printf("did wait\n");
        }
        assert(errno == ECHILD);
        return 0;
    }

    for (int i = 0; i < 6; i++) {
        if (fork() == 0) {
            sleep(1);
            return 0;
        }
    }

    if (execl("/proc/self/exe", *argv, "-w", NULL)) {
        perror("execl");
        return 1;
    }
}
