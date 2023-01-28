#include <stdio.h>
#include <unistd.h>

void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s <seconds>\n", argv[0]);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        print_usage(argv);
        return 0;
    }

    unsigned int secs;
    if (sscanf(argv[1], "%u", &secs) != 1) {
        print_usage(argv);
        return 1;
    }

    // Maybe should do something about possible interruptions
    sleep(secs);
    return 0;
}
