#include <signal.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <pid>\n", argv[0]);
        return 0;
    }

    pid_t pid;
    if (sscanf(argv[1], "%d", &pid) != 1) {
        printf("Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    if (kill(pid, SIGTERM)) {
        perror("kill");
        return 1;
    }

    return 0;
}