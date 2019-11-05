#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr = { 0 };
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/.socket_test.socket", sizeof(addr.sun_path));

    if (connect(fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        return 1;
    }

    char message_buf[50];
    if (read(fd, message_buf, 50) == -1) {
        perror("read");
        return 1;
    }

    fprintf(stderr, "%s\n", message_buf);

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    return 0;
}