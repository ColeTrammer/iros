#include <fcntl.h>
#include <graphics/font.h>
#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <liim/pointers.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "server.h"
#include "window.h"

int main() {
    umask(0002);

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

    uint32_t *raw_pixels =
        static_cast<uint32_t *>(mmap(NULL, sz.x * sz.y * sizeof(uint32_t) * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0));
    if (raw_pixels == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    auto front_buffer = PixelBuffer::wrap(raw_pixels, sz.x, sz.y);
    front_buffer->clear();

    auto back_buffer = PixelBuffer::wrap(raw_pixels + sz.x * sz.y, sz.x, sz.y);
    back_buffer->clear();

    Server server(fb, front_buffer, back_buffer);
    server.start();

    return 0;
}
