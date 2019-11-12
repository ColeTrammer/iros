#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <kernel/hal/x86_64/drivers/vga.h>

int main() {
    int fb = open("/dev/fb0", O_RDWR);
    assert(fb != -1);

    int width;
    int height;

    assert(ioctl(fb, SGWIDTH, &width) == 0);
    assert(ioctl(fb, SGHEIGHT, &height) == 0);

    uint16_t *vga_buffer = mmap(NULL, width * height * sizeof(uint16_t), PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    assert(vga_buffer != MAP_FAILED);

    for (int i = 0; i < width; i++) {
        vga_buffer[VGA_INDEX(height - 1, i)] = VGA_ENTRY('*', VGA_COLOR_RED, VGA_COLOR_WHITE);
    }
    return 0;
}