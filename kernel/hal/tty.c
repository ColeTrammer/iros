#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>

#include <kernel/hal/arch.h>
#include HAL_ARCH_SPECIFIC(drivers/vga.h)

#include <kernel/hal/tty.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/vfs.h>

#include <kernel/sched/process_sched.h>

static ssize_t tty_write(struct device *tty, struct file *file, const void *buffer, size_t len) {
    struct tty_data *data = (struct tty_data*) tty->private;
    const char *str = (const char*) buffer;

    if (file->position != 0) {
        return -EINVAL;
    }

    spin_lock(&data->lock);

    if (data->y >= data->y_max) {
        data->y = 0;
    }

    for (size_t i = 0; str[i] != '\0' && i < len; i++) {
        if (str[i] == '\x1B') {
            i++;
            if (str[i] != '[') {
                while (!isalpha(str[i])) { i++; }
                continue;
            }

            i++;

            int nums[3] = { -1, -1, -1 };
            nums[0] = atoi(str + i);
            while (isdigit(str[i])) { i++; }

            if (str[i] == ';') {
                i++;
                nums[1] = atoi(str + i);

                while (isdigit(str[i])) { i++; }

                if (str[i] == ';') {
                    i++;
                    nums[2] = atoi(str + i);

                    while (isdigit(str[i])) { i++; }
                }
            }

            /* Set Cursor Command */
            if (str[i] == 'H') {
                data->y = nums[0];
                data->x = nums[1];
                set_vga_cursor(data->y, data->x);
            }

            /* Clear Screen Command */
            if (str[i] == 'J') {
                /* Clear entire screen */
                if (nums[0] == 2) {
                    for (size_t r = 0; r < data->y_max; r++) {
                        for (size_t c = 0; c < data->x_max; c++) {
                            write_vga_buffer(r, c, ' ', false);
                        }
                    }
                }
            }

            /* Set Attribute Mode */
            if (str[i] == 'm') {
                for (size_t i = 0; i < 3; i++) {
                    switch (nums[i]) {
                        case 0:
                        /* Reset attributes */
                            set_vga_foreground(VGA_COLOR_LIGHT_GREY);
                            set_vga_background(VGA_COLOR_BLACK);
                            break;
                        case 30:
                            set_vga_foreground(VGA_COLOR_BLACK);
                            break;
                        case 31:
                            set_vga_foreground(VGA_COLOR_RED);
                            break;
                        case 32:
                            set_vga_foreground(VGA_COLOR_GREEN);
                            break;
                        case 33:
                            set_vga_foreground(VGA_COLOR_YELLOW);
                            break;
                        case 34:
                            set_vga_foreground(VGA_COLOR_BLUE);
                            break;
                        case 35:
                            set_vga_foreground(VGA_COLOR_MAGENTA);
                            break;
                        case 36:
                            set_vga_foreground(VGA_COLOR_CYAN);
                            break;
                        case 37:
                            set_vga_foreground(VGA_COLOR_LIGHT_GREY);
                            break;
                        case 40:
                            set_vga_background(VGA_COLOR_BLACK);
                            break;
                        case 41:
                            set_vga_background(VGA_COLOR_RED);
                            break;
                        case 42:
                            set_vga_background(VGA_COLOR_GREEN);
                            break;
                        case 43:
                            set_vga_background(VGA_COLOR_YELLOW);
                            break;
                        case 44:
                            set_vga_background(VGA_COLOR_BLUE);
                            break;
                        case 45:
                            set_vga_background(VGA_COLOR_MAGENTA);
                            break;
                        case 46:
                            set_vga_background(VGA_COLOR_CYAN);
                            break;
                        case 47:
                            set_vga_foreground(VGA_COLOR_LIGHT_GREY);
                            break;
                        default:
                            break;
                    }
                }
            }

        } else if (str[i] == '\n' || data->x >= data->x_max) {
            while (data->x < data->x_max) {
                write_vga_buffer(data->y, data->x++, ' ', false);
            }
            data->y++;
            data->x = 0;
            if (data->y >= data->y_max) {
                for (size_t r = 0; r < data->y_max - 1; r++) {
                    for (size_t c = 0; c < data->x_max; c++) {
                        write_vga_buffer(r, c, get_vga_buffer(r + 1, c), true);
                    }
                }
                for (size_t i = 0; i < data->x_max; i++) {
                    write_vga_buffer(data->y_max - 1, i, ' ', false);
                }
                data->y--;
            }
        } else {
            write_vga_buffer(data->y, data->x++, str[i], false);
        }
    }

    set_vga_cursor(data->y, data->x);

    spin_unlock(&data->lock);
    return (ssize_t) len;
}

static ssize_t tty_read(struct device *tty, struct file *file, void *buffer, size_t len) {
    struct tty_data *data = (struct tty_data*) tty->private;

    if (file->position != 0) {
        return -EINVAL;
    }

    if (data->input_buffer == NULL) {
        data->input_buffer = calloc(data->input_buffer_length, sizeof(char));

        size_t i = 0;
        size_t start_x = data->x;
        while(1) {
            fs_read(data->keyboard, &data->key_buffer, sizeof(struct key_event));

            if (data->key_buffer.flags & KEY_UP) {
                continue;
            }

            size_t start_pos = data->x - start_x;

            if (data->key_buffer.key == KEY_BACKSPACE) {
                if (start_pos > 0) {
                    if (i == start_pos) {
                        data->x--;
                        tty_write(tty, file, " ", 1);
                        data->x--;
                    } else {
                        memmove(data->input_buffer + start_pos - 1, data->input_buffer + start_pos, i - start_pos);
                        data->input_buffer[i - 1] = ' ';

                        size_t saved_x = --data->x;
                        tty_write(tty, file, data->input_buffer + start_pos - 1, i - start_pos + 1);
                        data->x = saved_x;
                    }

                    set_vga_cursor(data->y, data->x);
                    i--;
                }
                continue;
            }

            if (data->key_buffer.key == KEY_DELETE) {
                if (start_pos < i) {
                    memmove(data->input_buffer + start_pos, data->input_buffer + start_pos + 1, i - start_pos - 1);
                    data->input_buffer[i - 1] = ' ';

                    size_t saved_x = data->x;
                    tty_write(tty, file, data->input_buffer + start_pos, i - start_pos);
                    data->x = saved_x;

                    set_vga_cursor(data->y, data->x);
                    i--;
                }
                continue;
            }

            if (data->key_buffer.key == KEY_CURSOR_LEFT) {
                if (data->x > start_x) {
                    data->x--;
                    set_vga_cursor(data->y, data->x);
                }
                continue;
            }

            if (data->key_buffer.key == KEY_CURSOR_RIGHT) {
                if (data->x < start_x + i) {
                    data->x++;
                    set_vga_cursor(data->y, data->x);
                }
                continue;
            }

            if (data->key_buffer.ascii == '\n') {
                data->input_buffer[i++] = '\n';
                data->x = start_x + i;
                tty_write(tty, file, &data->key_buffer.ascii, 1);
                break;
            }

            if (data->key_buffer.ascii == '\0') {
                continue;
            }

            if (start_pos == i) {
                data->input_buffer[start_pos] = data->key_buffer.ascii;
                tty_write(tty, file, &data->key_buffer.ascii, 1);
            } else {
                char *copy = calloc(i - start_pos + 1, sizeof(char));
                memcpy(copy, data->input_buffer + start_pos, i - start_pos);
                data->input_buffer[start_pos] = data->key_buffer.ascii;
                data->input_buffer[start_pos + 1] = '\0';
                strcat(data->input_buffer, copy);
                free(copy);

                size_t saved_x = data->x;
                tty_write(tty, file, data->input_buffer + start_pos, strlen(data->input_buffer + start_pos));
                data->x = saved_x + 1;
                set_vga_cursor(data->y, data->x);
            }

            i++;

            if (i >= data->input_buffer_length - 1) {
                data->input_buffer_length *= 2;
                data->input_buffer = realloc(data->input_buffer, data->input_buffer_length);
            }
        }

        data->input_buffer[i] = '\0';
        data->input_buffer_offset = 0;
    }

    char *buf = (char*) buffer;

    int i;
    for (i = 0; data->input_buffer_offset < data->input_buffer_length - 1 && i < (int) len; i++) {
        buf[i] = data->input_buffer[data->input_buffer_offset++];

        if (buf[i] == '\n') {
            /* Could instead get more input */
            free(data->input_buffer);
            data->input_buffer = NULL;
            i++;
            break;
        }
    }

    return (ssize_t) i;
}

static void tty_add(struct device *tty) {
    struct tty_data *data = (struct tty_data*) tty->private;

    for (size_t r = 0; r < data->y_max; r++) {
        for (size_t c = 0; c < data->x_max; c++) {
            write_vga_buffer(r, c, ' ', false);
        }
    }

    set_vga_cursor(data->y, data->x);
}

static void tty_remove(struct device *tty) {
    struct tty_data *data = (struct tty_data*) tty->private;
    
    fs_close(data->keyboard);
    
    free(data->input_buffer);
    free(data->buffer);
    free(data);
}

struct device_ops tty_ops = {
    NULL, tty_read, tty_write, NULL, tty_add, tty_remove
};

void init_tty_device(dev_t dev) {
    struct device *device = malloc(sizeof(struct device));
    device->device_number = dev;
    strcpy(device->name, "tty");
    device->ops = &tty_ops;
    device->type = S_IFCHR;

    int error = 0;
    struct tty_data *data = malloc(sizeof(struct tty_data));
    data->keyboard = fs_open("/dev/keyboard", &error);
    assert(error == 0);
    data->input_buffer_length = 0x1000;
    data->input_buffer_offset = -1;
    data->input_buffer = NULL;
    data->x = 0;
    data->y = 0;
    data->x_max = DEFAULT_TTY_WIDTH;
    data->y_max = DEFUALT_TTY_HEIGHT;
    data->buffer = malloc(DEFAULT_TTY_WIDTH * DEFUALT_TTY_HEIGHT);
    init_spinlock(&data->lock);
    device->private = data;

    dev_add(device, device->name);
}