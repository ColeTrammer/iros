#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <window_server/message.h>

int main()
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(fd != -1);

    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/.window_server.socket");

    assert(connect(fd, (sockaddr*) &addr, sizeof(sockaddr_un)) == 0);

    uint8_t message_buffer[4096];
    memset(message_buffer, 1, 4096);
    assert(read(fd, &message_buffer, 4096) != -1);

    auto* message = reinterpret_cast<WindowServerMessage*>(message_buffer);

    fprintf(stderr, "%*s\n", message->data_len(), message->data());

    for (;;) {
        sleep(1);
    }

    close(fd);
    return 0;
}