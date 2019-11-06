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

    unlink(addr.sun_path);

    if (bind(fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(fd, 5) == -1) {
        perror("listen");
        return 1;
    }

    for (;;) {
        struct sockaddr_un client_addr = { 0 };
        socklen_t client_addr_len = sizeof(struct sockaddr_un);

        int cfd = accept(fd, (struct sockaddr*) &client_addr, &client_addr_len);
        if (cfd == -1) {
            perror("accept");
            return 1;
        }

        if (write(cfd, "Hello", 6) != 6) {
            perror("write");
            return 1;
        }

        char message_buf[50];
        if (read(cfd, message_buf, 50) == -1) {
            perror("read");
            return 1;
        }

        fprintf(stderr, "Recieved: %s\n", message_buf);

        if (close(cfd) == -1) {
            perror("close");
            return 1;
        }
    }

    return 0;
}