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

    fprintf(serial, "\033[32mProcess \033[37m(\033[34m %d \033[37m): \033[36mserial\033[37m: [ %s ]\n", getpid(), argv[1]);
    fclose(serial);

    return 0;
}
