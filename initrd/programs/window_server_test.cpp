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
#include <window_server/connection.h>
#include <window_server/window.h>

int main() {
    WindowServer::Connection connection;
    auto window = connection.create_window(100, 100, 200, 200);

    int cnt = 0;
    connection.set_draw_callback(window, [&](auto& pixels) {
        Renderer renderer(*pixels);
        renderer.fill_rect(50 + cnt, 50 + cnt, 50, 50);
        if (++cnt >= 100) {
            exit(0);
        }
    });

    for (;;) {
        pause();
    }
    return 0;
}