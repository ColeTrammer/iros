#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <dest>\n", argv[0]);
        return 0;
    }

    int source = open(argv[1], O_RDONLY);
    int dest = open(argv[2], O_WRONLY | O_CREAT, 0644);

    if (source == -1 || dest == -1) {
        fprintf(stderr, "cp: could not open files.\n");
    }

    char buf[BUFSIZ];

    ssize_t len;
    while ((len = read(source, buf, BUFSIZ)) > 0) {
        write(dest, buf, len);
    }

    close(source);
    close(dest);

    return 0;
}