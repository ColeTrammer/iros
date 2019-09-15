#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <message>\n", argv[0]);
        return 0;
    }

    FILE *serial = fopen("/dev/serial", "w");
    if (serial == NULL) {
        perror("serial");
    }

    fprintf(serial, "Process %d: [ %s ]\n", getpid(), argv[1]);
    fclose(serial);

    return 0;
}