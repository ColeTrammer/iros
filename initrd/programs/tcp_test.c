#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in conn = { 0 };
    conn.sin_family = AF_INET;
    conn.sin_port = htons(80);
    // conn.sin_addr.s_addr = inet_addr("50.87.248.178");
    conn.sin_addr.s_addr = inet_addr("172.105.70.201");
    // conn.sin_addr.s_addr = INADDR_LOOBACK;

    if (connect(fd, (const struct sockaddr*) &conn, sizeof(struct sockaddr_in)) == -1) {
        perror("connect");
        return 1;
    }

    char *http_request = "GET / HTTP/1.0\r\nHost: www.serenityos.org\r\n\r\n";
    ssize_t len = strlen(http_request);

    if (write(fd, http_request, len) != len) {
        perror("write");
        return 1;
    }

    char buf[2048];
    if (read(fd, buf, 2048) == -1) {
        perror("read");
        return 1;
    }

    FILE *s = fopen("/dev/serial", "w");
    assert(s);

    fputs(buf, s);
    fclose(s);

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    return 0;
}