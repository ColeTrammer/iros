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

    if (data->input_buffer_offset == -1) {
        int i = 0;
        do {
            fs_read(data->keyboard, &data->key_buffer, sizeof(struct key_event));

            if (data->key_buffer.flags & KEY_UP) {
                continue;
            }

            if (data->key_buffer.key == KEY_BACKSPACE) {
                data->x--;
                tty_write(tty, " ", 1);
                data->x--;
                i--;
            }

            if (data->key_buffer.ascii == '\0') {
                continue;
            }

            tty_write(tty, &data->key_buffer.ascii, 1);
            data->input_buffer[i++] = data->key_buffer.ascii;

            if (i >= data->input_buffer_length - 1) {
                data->input_buffer_length *= 2;
                data->input_buffer = realloc(data->input_buffer, data->input_buffer_length);
            }
        } while (data->key_buffer.ascii != '\n');

        data->input_buffer[i] = '\0';
        data->input_buffer_offset = 0;
    }

    char *buf = (char*) buffer;

    int i;
    for (i = 0; data->input_buffer_offset < data->input_buffer_length - 1 && i < (int) len - 1; i++) {
        buf[i] = data->input_buffer[data->input_buffer_offset++];

        if (buf[i] == '\n') {
            /* Could instead get more input */
            data->input_buffer_offset = -1;
            break;
        }
    }

    buf[i] = '\0';

    return (ssize_t) i;
}

static void tty_remove(struct device *tty) {
    struct tty_data *data = (struct tty_data*) tty->private;
    
    fs_close(data->keyboard);
    
    free(data->input_buffer);
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
    data->input_buffer_length = 0x1000;
    data->input_buffer_offset = -1;
    data->input_buffer = malloc(data->input_buffer_length);
    data->x = 0;
    data->y = 0;
    data->x_max = DEFAULT_TTY_WIDTH;
    data->y_max = DEFUALT_TTY_HEIGHT;
    data->buffer = malloc(DEFAULT_TTY_WIDTH * DEFUALT_TTY_HEIGHT);
    init_spinlock(&data->lock);
    device->private = data;

    dev_add(device, device->name);
}