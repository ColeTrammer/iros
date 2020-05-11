#include <signal.h>
#include <stdio.h>

static void print_usage(char **argv) {
    printf("Usage: %s <signum> <pid>\n", argv[0]);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage(argv);
        return 0;
    }

    int signum;
    if (sscanf(argv[1], "%d", &signum) != 1) {
        print_usage(argv);
        return 1;
    }

    pid_t pid;
    if (sscanf(argv[2], "%d", &pid) != 1) {
        print_usage(argv);
        return 1;
    }

    if (kill(pid, signum)) {
        perror("kill");
        return 1;
    }

    return 0;
}
