#include <fcntl.h>
#include <graphics/font.h>
#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <liim/pointers.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "server.h"
#include "window.h"

int main()
{
    int fb = open("/dev/fb0", O_RDWR);
    if (fb == -1) {
        perror("open");
        return 1;
    }

    screen_res sz = { 1024, 768 };
    if (ioctl(fb, SSRES, &sz) == -1) {
        perror("ioctl");
        return 1;
    }

    uint32_t *raw_pixels = static_cast<uint32_t*>(mmap(NULL, sz.x * sz.y * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0));
    if (raw_pixels == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    auto pixels = PixelBuffer::wrap(raw_pixels, sz.x, sz.y);
    pixels->clear();

    Window window = Window(Rect(200, 200, 100, 100));

    auto render_window = [&](const Window& window) {
        for (int x = window.rect().x(); x < window.rect().x() + window.rect().width(); x++) {
            for (int y = window.rect().y(); y < window.rect().y() + window.rect().height(); y++) {
                pixels->put_pixel(x, y, window.buffer()->get_pixel(x - window.rect().x(), y - window.rect().y()));
            }
        }
    };

    auto renderer = Renderer(window.buffer());
    renderer.fill_rect(0, 0, 100, 100);

    render_window(window);

    Server server;
    server.start();

    return 0;
}