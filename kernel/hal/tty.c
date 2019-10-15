#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>

#include <kernel/hal/arch.h>
#include HAL_ARCH_SPECIFIC(drivers/vga.h)

#include <kernel/hal/tty.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/vfs.h>

#include <kernel/sched/process_sched.h>

#define CONTROL_MASK 0x1F
#define CONTROL_KEY(c) ((c) & CONTROL_MASK)

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

    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\0') {
            continue;
        }

        if (str[i] == '\x1B') {
            i++;
            if (str[i] != '[') {
                while (!isalpha(str[i])) { i++; }
                continue;
            }

            i++;

            bool starts_with_q = false;
            if (str[i] == '?') {
                starts_with_q = true;
                i++;
            }

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

            /* Question commands */
            if (starts_with_q) {
                if (nums[0] == 25) {
                    /* Hide cursor command */
                    if (str[i] == 'l') {
                        vga_disable_cursor();
                    }

                    /* Show cursor command */
                    if (str[i] == 'h') {
                        vga_enable_cursor();
                    }
                }
            }

            /* Set Cursor Command */
            if (str[i] == 'H') {
                if (--nums[0] < 0) {
                    nums[0] = 0;
                } else if (nums[0] >= (int) data->y_max) {
                    nums[0] = data->y_max - 1;
                }

                if (--nums[1] < 0) {
                    nums[1] = 0;
                } else if (nums[1] >= (int) data->x_max) {
                    nums[1] = data->x_max - 1;
                }

                data->y = nums[0];
                data->x = nums[1];
                set_vga_cursor(data->y, data->x);
            }

            /* Clear Line Command */
            if (str[i] == 'K') {
                if (nums[0] == 0) {
                    for (size_t c = data->x; c < data->x_max; c++) {
                        write_vga_buffer(data->y, c, ' ', false);
                    }
                }
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
                        case 7:
                        /* Invert colors */
                            swap_vga_colors();
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
                        case 39:
                            set_vga_foreground(VGA_COLOR_LIGHT_GREY);
                            break;
                        case 40:
                        case 49:
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

        } else if (str[i] == '\r') {
            data->x = 0;
        } else if (str[i] == '\n' || data->x >= data->x_max) {
            if (data->config.c_oflag & OPOST) {
                data->x = 0;
            }
            data->y++;
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

    char *buf = (char*) buffer;
    if (!(data->config.c_lflag & ICANON)) {
        for (size_t i = 0; i < len;) {
            if (data->input_in_escape) {
                assert(data->input_escape);
                char c = data->input_escape[data->input_buffer_offset];
                if (c == '\0') {
                    data->input_in_escape = false;
                    data->input_escape = NULL;
                    data->input_buffer_offset = 0;
                    continue;
                }

                buf[i++] = data->input_escape[data->input_buffer_offset++];
                continue;
            }

            fs_read(data->keyboard, &data->key_buffer, sizeof(struct key_event));
            if (data->key_buffer.flags & KEY_DOWN) {
                if (data->key_buffer.key == KEY_CURSOR_UP) {
                    data->input_in_escape = true;
                    data->input_escape = "\x1B[A";
                    data->input_buffer_offset = 0;
                }

                if (data->key_buffer.key == KEY_CURSOR_DOWN) {
                    data->input_in_escape = true;
                    data->input_escape = "\x1B[B";
                    data->input_buffer_offset = 0;
                }

                if (data->key_buffer.key == KEY_CURSOR_RIGHT) {
                    data->input_in_escape = true;
                    data->input_escape = "\x1B[C";
                    data->input_buffer_offset = 0;
                }

                if (data->key_buffer.key == KEY_CURSOR_LEFT) {
                    data->input_in_escape = true;
                    data->input_escape = "\x1B[D";
                    data->input_buffer_offset = 0;
                }

                if (data->key_buffer.key == KEY_DELETE) {
                    data->input_in_escape = true;
                    data->input_escape = "\x1B[3~";
                    data->input_buffer_offset = 0;
                }

                if (data->key_buffer.key == KEY_PAGE_UP) {
                    data->input_in_escape = true;
                    data->input_escape = "\x1B[5~";
                    data->input_buffer_offset = 0;
                }

                if (data->key_buffer.key == KEY_PAGE_DOWN) {
                    data->input_in_escape = true;
                    data->input_escape = "\x1B[6~";
                    data->input_buffer_offset = 0;
                }

                if (data->key_buffer.key == KEY_HOME) {
                    data->input_in_escape = true;
                    data->input_escape = "\x1B[7~";
                    data->input_buffer_offset = 0;
                }

                if (data->key_buffer.key == KEY_END) {
                    data->input_in_escape = true;
                    data->input_escape = "\x1B[8~";
                    data->input_buffer_offset = 0;
                }

                if ((data->key_buffer.flags & KEY_CONTROL_ON) && (('a' <= data->key_buffer.ascii && data->key_buffer.ascii <= 'z') || ('A' <= data->key_buffer.ascii && data->key_buffer.ascii <= 'Z'))) {
                    data->key_buffer.ascii &= 0x1F;
                }

                if (data->key_buffer.ascii == '\r' && data->config.c_iflag & ICRNL) {
                    data->key_buffer.ascii = '\n';
                }
                
                if (data->key_buffer.ascii != '\0') {
                    buf[i++] = data->key_buffer.ascii;
                }
            }
        }
        return len;
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
                        if (data->config.c_lflag & ECHO) {
                            data->x--;
                            tty_write(tty, file, " ", 1);
                        }
                        data->x--;
                    } else {
                        memmove(data->input_buffer + start_pos - 1, data->input_buffer + start_pos, i - start_pos);
                        data->input_buffer[i - 1] = ' ';

                        if (data->config.c_lflag & ECHO) {
                            size_t saved_x = --data->x;
                            tty_write(tty, file, data->input_buffer + start_pos - 1, i - start_pos + 1);
                            data->x = saved_x;
                        }
                    }

                    if (data->config.c_lflag & ECHO) {
                        set_vga_cursor(data->y, data->x);
                        i--;
                    }
                }
                continue;
            }

            if (data->key_buffer.key == KEY_DELETE) {
                if (start_pos < i) {
                    memmove(data->input_buffer + start_pos, data->input_buffer + start_pos + 1, i - start_pos - 1);
                    data->input_buffer[i - 1] = ' ';

                    if (data->config.c_lflag & ECHO) {
                        size_t saved_x = data->x;
                        tty_write(tty, file, data->input_buffer + start_pos, i - start_pos);
                        data->x = saved_x;

                        set_vga_cursor(data->y, data->x);
                        i--;
                    }
                }
                continue;
            }

            if (data->key_buffer.key == KEY_CURSOR_LEFT) {
                if (data->x > start_x && data->config.c_lflag & ECHO) {
                    data->x--;
                    set_vga_cursor(data->y, data->x);
                }
                continue;
            }

            if (data->key_buffer.key == KEY_CURSOR_RIGHT && data->config.c_lflag & ECHO) {
                if (data->x < start_x + i) {
                    data->x++;
                    set_vga_cursor(data->y, data->x);
                }
                continue;
            }

            if (data->key_buffer.ascii == '\n' || data->key_buffer.ascii == '\r') {
                if (data->config.c_iflag & ICRNL) {
                    data->key_buffer.ascii = '\n';
                }

                data->input_buffer[i++] = data->key_buffer.ascii;
                
                if (data->config.c_lflag & ECHO) {
                    data->x = start_x + i;
                    tty_write(tty, file, &data->key_buffer.ascii, 1);
                }
                
                break;
            }

            if (data->key_buffer.flags & KEY_CONTROL_ON) {
                data->key_buffer.ascii &= CONTROL_MASK;
            }

            // Send interrupts on ^C
            if (data->key_buffer.ascii == CONTROL_KEY('c') && data->config.c_lflag & ISIG) {
                // Discard input buffer
                free(data->input_buffer);
                data->input_buffer = NULL;

                tty_write(tty, file, "^C", 2);

                // Signal foreground process group
                signal_process_group(data->pgid, SIGINT);
                return -EINTR;
            }

            /* Send EOF by returning 0 for read */
            if (data->key_buffer.ascii == CONTROL_KEY('d')) {
                if (i == 0) {
                    free(data->input_buffer);
                    data->input_buffer = NULL;
                    return 0;
                }
            }

            if (iscntrl(data->key_buffer.ascii)) {
                continue;
            }

            if (start_pos == i) {
                data->input_buffer[start_pos] = data->key_buffer.ascii;

                if (data->config.c_lflag & ECHO) {
                    tty_write(tty, file, &data->key_buffer.ascii, 1);
                }
            } else {
                char *copy = calloc(i - start_pos + 1, sizeof(char));
                memcpy(copy, data->input_buffer + start_pos, i - start_pos);
                data->input_buffer[start_pos] = data->key_buffer.ascii;
                data->input_buffer[start_pos + 1] = '\0';
                strcat(data->input_buffer, copy);
                free(copy);

                if (data->config.c_lflag & ECHO) {
                    size_t saved_x = data->x;
                    tty_write(tty, file, data->input_buffer + start_pos, strlen(data->input_buffer + start_pos));
                    data->x = saved_x + 1;
                    set_vga_cursor(data->y, data->x);
                }
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

static int tty_ioctl_termios_get_winsize(struct device *tty, struct winsize *ws) {
    struct tty_data *data = tty->private;

    ws->ws_row = data->y_max;
    ws->ws_col = data->x_max;

    return 0;
}

static int tty_ioctl_termios_get(struct device *tty, struct termios *termios_p) {
    struct tty_data *data = tty->private;
    
    spin_lock(&data->lock);
    memcpy(termios_p, &data->config, sizeof(struct termios));
    spin_unlock(&data->lock);

    return 0;
}

static int tty_ioctl_termios_set(struct device *tty, struct termios *termios_p) {
    struct tty_data *data = tty->private;
    
    spin_lock(&data->lock);
    memcpy(&data->config, termios_p, sizeof(struct termios));
    spin_unlock(&data->lock);

    return 0;
}

static int tty_ioctl(struct device *tty, unsigned long request, void *argp) {
    struct tty_data *data = tty->private;

    switch (request) {
        case TIOCGWINSZ:
            return tty_ioctl_termios_get_winsize(tty, argp);
        case TIOCSPGRP:
            spin_lock(&data->lock);
            data->pgid = *((pid_t*) argp);
            spin_unlock(&data->lock);
            return 0;
        case TIOCGPGRP:
            return data->pgid;
        case TCGETS:
            return tty_ioctl_termios_get(tty, argp);
        case TCSETSF:
            /* Flushes input */
            free(data->input_buffer);
            data->input_buffer = NULL;
            // Fall through
        case TCSETSW:
            /* Would flush output but there is not buffering yet */
            // Fall through
        case TCSETS:
            return tty_ioctl_termios_set(tty, argp);
        default:
            return -ENOTTY;
    }
}

static struct device_ops tty_ops = {
    NULL, tty_read, tty_write, NULL, tty_add, tty_remove, tty_ioctl
};

static cc_t tty_default_control_characters[NCCS] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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
    data->config.c_iflag = ICRNL | IXON;
    data->config.c_oflag = OPOST;
    data->config.c_cflag = CS8;
    data->config.c_lflag = ECHO | ICANON | IEXTEN | ISIG;
    memcpy(data->config.c_cc, tty_default_control_characters, NCCS * sizeof(cc_t));
    data->pgid = 2; // Hard code value of shell (should be something else)
    device->private = data;

    dev_add(device, device->name);
}