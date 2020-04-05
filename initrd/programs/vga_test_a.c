#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <kernel/hal/input.h>
#include <kernel/hal/x86_64/drivers/vga.h>

int main() {
#ifdef __os_2__
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
            char buf[50] = { 0 };
            read(sfd, buf, 50);

            write(dfd, buf, strlen(buf));

            sleep(1);
        }
    }

    int kfd = open("/dev/keyboard", O_RDONLY);
    struct key_event event;

    int x = 0;
    int y = 0;
    for (;;) {
        char c;
        while (read(mfd, &c, 1) == 1) {
            vga_buffer[VGA_INDEX(y, x)] = VGA_ENTRY(c, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            x++;
        }

        if (read(kfd, &event, sizeof(struct key_event)) != 0) {
            if (event.ascii != '\0' && event.flags & KEY_DOWN) {
                if (event.flags & KEY_CONTROL_ON) {
                    event.ascii &= 0x1F;
                }
                write(mfd, &event.ascii, 1);
            }
        }
    }
#endif /* __os_2__ */
    return 0;
}