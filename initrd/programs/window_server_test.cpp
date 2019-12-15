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

int main() {
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
    int shm_front = shm_open(created_data.shared_buffer_path, O_RDWR, 0);
    created_data.shared_buffer_path[strlen(created_data.shared_buffer_path) - 1]++;
    int shm_back = shm_open(created_data.shared_buffer_path, O_RDWR, 0);

    void* front_raw_memory = mmap(nullptr, created_data.shared_buffer_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm_front, 0);
    void* back_raw_memory = mmap(nullptr, created_data.shared_buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_back, 0);
    close(shm_front);
    close(shm_back);

    auto front_pixels = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(front_raw_memory), 200, 200);
    auto back_pixels = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(back_raw_memory), 200, 200);

    for (int i = 0; i < 100; i++) {
        back_pixels->clear();
        Renderer renderer(back_pixels);
        renderer.fill_rect(50 + i, 50 + i, 50, 50);

        auto swap_buffer_request = WindowServer::Message::SwapBufferRequest::create(created_data.window_id);
        assert(write(fd, swap_buffer_request.get(), swap_buffer_request->total_size()) != -1);

        auto temp = back_pixels;
        back_pixels = front_pixels;
        front_pixels = temp;

        for (int i = 0; i < 100000; i++) {
            getpid();
        }
    }

    close(fd);
    return 0;
}