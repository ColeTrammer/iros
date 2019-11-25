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

    {
        auto message = WindowServer::Message::CreateWindowRequest::create(100, 100, 200, 200);
        assert(write(fd, message.get(), message->total_size()) != -1);
    }

    uint8_t message_buffer[4096];
    auto* message = reinterpret_cast<WindowServer::Message*>(message_buffer);
    read(fd, message_buffer, 4096);
    assert(message->type == WindowServer::Message::Type::CreateWindowResponse);

    WindowServer::Message::CreateWindowResponse& created_data = message->data.create_window_response;
    int shm = shm_open(created_data.shared_buffer_path, O_RDWR, 0);

    void* raw_memory = mmap(nullptr, created_data.shared_buffer_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
    close(shm);

    auto pixels = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(raw_memory), 200, 200);
    Renderer renderer(pixels);
    renderer.fill_rect(50, 50, 50, 50);

    sleep(5);

    close(fd);
    return 0;
}