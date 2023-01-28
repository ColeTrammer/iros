#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (ret) {
        perror("socket_pair_test: socketpair");
        return 1;
    }

    const char *message = "Hello, World!\n";
    if (send(fds[1], message, strlen(message) + 1, 0) < 0) {
        perror("socket_pair_test: send");
        return 1;
    }

    char recv_buffer[124];
    if (recv(fds[0], recv_buffer, sizeof(recv_buffer), 0) < 0) {
        perror("socket_pair_test: recv");
        return 1;
    }

    fputs(recv_buffer, stdout);

    close(fds[0]);
    close(fds[1]);
    return 0;
}
