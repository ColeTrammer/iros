#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

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

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            vga_buffer[VGA_INDEX(j, i)] = VGA_ENTRY(' ', VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }
    }

    int mfd = posix_openpt(O_RDWR);
    assert(mfd != -1);

    int dfd = open("/dev/serial", O_WRONLY);

    if (fork() == 0) {
        int sfd = open(ptsname(mfd), O_RDWR);
        assert(sfd != -1);

        for (;;) {
            write(sfd, "A", 1);

            char c;
            read(sfd, &c, 1);
            write(dfd, &c, 1);
            sleep(1);
        }
    }


    for (;;) {
        char cs[50] = { 0 };
        write(mfd, "BBB\n", 4);
        if (read(mfd, &cs, 50) == 0) {
            break;
        }

        write(dfd, &cs, strlen(cs));

        sleep(5);
    }

    return 0;
}