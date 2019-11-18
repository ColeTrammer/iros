#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int fd = STDIN_FILENO;
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd == -1) {
            perror("cat");
            return 1;
        }
    }

    char buf[BUFSIZ];
    ssize_t amount;
    while ((amount = read(fd, buf, BUFSIZ)) > 0) {
        if (write(STDOUT_FILENO, buf, amount) != amount) {
            perror("cat");
            return 1;
        }
    }

    if (amount != 0) {
        perror("cat");
        return 1;
    }

    return 0;
}