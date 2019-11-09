#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <socket> <message>\n", argv[0]);
        return 0;
    }

    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr = { 0 };
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        return 1;
    }

    size_t message_length = strlen(argv[2]) + 1;
    if (write(fd, argv[2], message_length) != (ssize_t) message_length) {
        perror("write");
        return 1;
    }

    char message_buf[500];
    int ret = read(fd, message_buf, 50);
    if (ret == -1) {
        perror("read");
        return 1;
    } else if (ret == 0) {
        return 1;
    }

    printf("%s\n", message_buf);

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    return 0;
}