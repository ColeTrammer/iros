#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(fd, (const struct sockaddr*) &addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(fd, 5) == -1) {
        perror("listen");
        return 1;
    }

    struct sockaddr_in client_addr = { 0 };
    socklen_t client_addr_len = sizeof(struct sockaddr_in);

    for (;;) {
        int clientfd = accept(fd, (struct sockaddr*) &client_addr, &client_addr_len);
        if (clientfd == -1) {
            perror("accept");
            return 1;
        }

        char buf[2048] = { 0 };
        ssize_t message_len = read(clientfd, buf, 2048);
        if (message_len == -1) {
            perror("read");
            return 1;
        }

        fprintf(stderr, "%s", buf);

        if (write(clientfd, "Test Message\n", 13) != 13) {
            perror("write");
            return 1;
        }

        if (close(clientfd) == -1) {
            perror("close");
            return 1;
        }
    }

    return 0;
}
