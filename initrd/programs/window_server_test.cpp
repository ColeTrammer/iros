#ifdef __os_2__

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
    auto window = connection.create_window(100, 100, 250, 250, "Window Server Test", WindowServer::WindowType::Application);

    int cnt = 0;
    connection.set_draw_callback(window, [&](auto& pixels) {
        Renderer renderer(*pixels);
        renderer.pixels().clear();
        renderer.fill_rect(50 + cnt, 50 + cnt, 50, 50, ColorValue::White);
        if (++cnt >= 100) {
            exit(0);
        }
    });

    for (;;) {
        window->draw();
        usleep(300000);
    }
    return 0;
}

#else
int main() {}
#endif /* __os_2__ */
