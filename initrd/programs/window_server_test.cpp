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

    uint8_t message_buffer[2048];
    WindowServerMessage* message = reinterpret_cast<WindowServerMessage*>(message_buffer);

    message->set_type(WindowServerMessage::Type::CreateWindow);
    message->set_data_len(sizeof(WindowServerMessage::CreateWindowData));

    WindowServerMessage::CreateWindowData* data = reinterpret_cast<WindowServerMessage::CreateWindowData*>(message->data());
    data->x = 100;
    data->y = 100;
    data->width = 200;
    data->height = 200;

    assert(write(fd, message, sizeof(WindowServerMessage) + message->data_len()) != -1);

    read(fd, message_buffer, 4096);
    assert(message->type() == WindowServerMessage::Type::CreatedWindow);

    WindowServerMessage::CreatedWindowData* created_data = reinterpret_cast<WindowServerMessage::CreatedWindowData*>(message->data());
    fprintf(stderr, "%s\n", created_data->shared_buffer_path);

    close(fd);
    return 0;
}