#include <assert.h>
#include <fcntl.h>
#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
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
    int shm = shm_open(created_data->shared_buffer_path, O_RDWR, 0);

    void* raw_memory = mmap(nullptr, created_data->shared_buffer_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
    fprintf(stderr, "addr: [ %#.16lX ]\n", (uintptr_t) raw_memory);
    fprintf(stderr, "size: [ %#.16lX ]\n", created_data->shared_buffer_size);
    close(shm);

    auto pixels = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(raw_memory), 200, 200);
    Renderer renderer(pixels);
    renderer.fill_rect(50, 50, 50, 50);

    close(fd);
    return 0;
}