#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include <kernel/hal/arch.h>
#include HAL_ARCH_SPECIFIC(drivers/vga.h)

#include <kernel/hal/tty.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/vfs.h>

#include <kernel/sched/process_sched.h>

static ssize_t tty_write(struct device *tty, const void *buffer, size_t len) {
    struct tty_data *data = (struct tty_data*) tty->private;
    const char *str = (const char*) buffer;

    spin_lock(&data->lock);

    if (data->y >= data->y_max) {
        data->y = 0;
    }

    for (size_t i = 0; str[i] != '\0' && i < len; i++) {
        if (str[i] == '\n' || data->x >= data->x_max) {
            while (data->x < data->x_max) {
                write_vga_buffer(data->y, data->x++, ' ');
            }
            data->y++;
            data->x = 0;
            if (data->y >= data->y_max) {
                for (size_t r = 0; r < data->y_max - 1; r++) {
                    for (size_t c = 0; c < data->x_max; c++) {
                        write_vga_buffer(r, c, get_vga_buffer(r + 1, c));
                    }
                }
                for (size_t i = 0; i < data->x_max; i++) {
                    write_vga_buffer(data->y_max - 1, i, ' ');
                }
                data->y--;
            }
        } else {
            write_vga_buffer(data->y, data->x++, str[i]);
        }
    }

    spin_unlock(&data->lock);
    return (ssize_t) len;
}

static ssize_t tty_read(struct device *tty, void *buffer, size_t len) {
    struct tty_data *data = (struct tty_data*) tty->private;

    uint8_t *buf = (uint8_t*) buffer;

    for (size_t i = 0; i < len - 1;) {
        fs_read(data->keyboard, &data->key_buffer, sizeof(struct key_event));

        if (data->key_buffer.flags & KEY_UP) {
            continue;
        }

        if (data->key_buffer.ascii == '\0') {
            continue;
        }

        buf[i++] = data->key_buffer.ascii;
        tty_write(tty, &data->key_buffer.ascii, 1);
    }

    buf[len - 1] = '\0';
    return (ssize_t) len;
}

static void tty_remove(struct device *tty) {
    struct tty_data *data = (struct tty_data*) tty->private;
    fs_close(data->keyboard);
    free(data->buffer);
    free(data);
}

struct device_ops tty_ops = {
    NULL, tty_read, tty_write, NULL, NULL, tty_remove
};

void init_tty_device(dev_t dev) {
    struct device *device = malloc(sizeof(struct device));
    device->device_number = dev;
    strcpy(device->name, "tty");
    device->ops = &tty_ops;

    int error = 0;
    struct tty_data *data = malloc(sizeof(struct tty_data));
    data->keyboard = fs_open("/dev/keyboard", &error);
    assert(error == 0);
    data->x = 0;
    data->y = 0;
    data->x_max = DEFAULT_TTY_WIDTH;
    data->y_max = DEFUALT_TTY_HEIGHT;
    data->buffer = malloc(DEFAULT_TTY_WIDTH * DEFUALT_TTY_HEIGHT);
    init_spinlock(&data->lock);
    device->private = data;

    dev_add(device, device->name);
}