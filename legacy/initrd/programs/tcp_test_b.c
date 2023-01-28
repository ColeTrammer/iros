#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int s1 = socket(AF_INET, SOCK_STREAM, 0);
    if (s1 < 0) {
        perror("tcp_test_b: socket");
        return 1;
    }

    int s2 = socket(AF_INET, SOCK_STREAM, 0);
    if (s2 < 0) {
        perror("tcp_test_b: socket");
        return 1;
    }

    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = 0, .sin_addr = { .s_addr = INADDR_ANY }, .sin_zero = { 0 } };
    if (bind(s1, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("tcp_test_b: bind");
        return 1;
    }

    socklen_t addrlen = sizeof(addr);
    if (getsockname(s1, (struct sockaddr*) &addr, &addrlen) < 0) {
        perror("tcp_test_b: getsockname");
        return 1;
    }
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (listen(s1, 5) < 0) {
        perror("tcp_test_b: listen");
        return 1;
    }

    if (connect(s2, (const struct sockaddr*) &addr, addrlen) < 0) {
        perror("tcp_test_b: connect");
        return 1;
    }

    int s3 = accept(s1, NULL, NULL);
    if (s3 < 0) {
        perror("tcp_test_b: accept");
        return 1;
    }

    char message[] = "Hello, World!";
    if (send(s2, message, sizeof(message) - 1, 0) < 0) {
        perror("tcp_test_b: send");
        return 1;
    }

    char buffer[1024] = { 0 };
    if (recv(s3, buffer, sizeof(buffer), 0) < 0) {
        perror("tcp_test_b: recv");
        return 1;
    }

    puts(buffer);

    if (close(s1) < 0) {
        perror("tcp_test_b: close");
        return 1;
    }

    if (close(s2) < 0) {
        perror("tcp_test_b: close");
        return 1;
    }

    if (close(s3) < 0) {
        perror("tcp_test_b: close");
        return 1;
    }

    return 0;
}
