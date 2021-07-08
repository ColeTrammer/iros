#include <assert.h>
#include <errno.h>
#include <search.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <termios.h>

#include <kernel/fs/cached_dirent.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/file.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/ptmx.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>
#include <kernel/util/mutex.h>

// #define PTMX_BLOCKING_DEBUG
// #define PTMX_SIGNAL_DEBUG
// #define PTMX_TERMIOS_DEBUG

#define PTMX_MAX 16

#define TTY_BUF_START 4096

#define CONTROL_MASK   0x1F
#define CONTROL_KEY(c) ((c) &CONTROL_MASK)

static struct termios default_termios = {
    ICRNL | IXON,
    OPOST,
    CS8,
    ECHO | ICANON | IEXTEN | ISIG,
    {
        CONTROL_KEY('d'),
        '\n',
        '\b',
        CONTROL_KEY('c'),
        '\025',
        1,
        CONTROL_KEY('\\'),
        CONTROL_KEY('q'),
        CONTROL_KEY('s'),
        CONTROL_KEY('z'),
        0,
    },
};

static struct fs_device *slaves[PTMX_MAX] = { 0 };
static struct fs_device *masters[PTMX_MAX] = { 0 };
static mutex_t lock = MUTEX_INITIALIZER(lock);

static void ptmx_log_termios(struct termios *termios) {
    (void) termios;

#ifdef PTMX_TERMIOS_DEBUG
    debug_log(
        "c_iflag: [ ICRNL=%d ISTRIP=%d IXON=%d IXOFF=%d IXANY=%d BRKINT=%d PARMRK=%d IGNBRK=%d IGNCR=%d IGNPAR=%d INLCR=%d INPCK=%d ]\n",
        !!(termios->c_iflag & ICRNL), !!(termios->c_iflag & ISTRIP), !!(termios->c_iflag & IXON), !!(termios->c_iflag & IXOFF),
        !!(termios->c_iflag & IXANY), !!(termios->c_iflag & BRKINT), !!(termios->c_iflag & PARMRK), !!(termios->c_iflag & IGNBRK),
        !!(termios->c_iflag & IGNCR), !!(termios->c_iflag & IGNPAR), !!(termios->c_iflag & INLCR), !!(termios->c_iflag & INPCK));
    debug_log("c_oflag: [ OPOST=%d ONLCR=%d OCRNL=%d ONOCR=%d ONLRET=%d OFDEL=%d OFILL=%d ]\n", !!(termios->c_oflag & OPOST),
              !!(termios->c_oflag & ONLCR), !!(termios->c_oflag & OCRNL), !!(termios->c_oflag & ONOCR), !!(termios->c_oflag & ONLRET),
              !!(termios->c_oflag & OFDEL), !!(termios->c_oflag & OFILL));
    debug_log("c_lflag: [ ECHO=%d ECHOE=%d ECHOK=%d ECHONL=%d ICANON=%d IEXTEN=%d ISIG=%d NOFLSH=%d TOSTOP=%d ]\n",
              !!(termios->c_lflag & ECHO), !!(termios->c_lflag & ECHOE), !!(termios->c_lflag & ECHOK), !!(termios->c_lflag & ECHONL),
              !!(termios->c_lflag & ICANON), !!(termios->c_lflag & IEXTEN), !!(termios->c_lflag & ISIG), !!(termios->c_lflag & NOFLSH),
              !!(termios->c_lflag & TOSTOP));
    debug_log("c_cc: [ VEOF=%d VEOL=%d VERASE=%d VINTR=%d VKILL=%d VMIN=%d VQUIT=%d VSTART=%d VSTOP=%d VSUSP=%d VITME=%d ]\n",
              termios->c_cc[VEOF], termios->c_cc[VEOL], termios->c_cc[VERASE], termios->c_cc[VINTR], termios->c_cc[VKILL],
              termios->c_cc[VMIN], termios->c_cc[VQUIT], termios->c_cc[VSTART], termios->c_cc[VSTOP], termios->c_cc[VSUSP],
              termios->c_cc[VTIME]);

#endif /* PTMX_TERMIOS_DEBUG */
}

static void slave_on_open(struct fs_device *device) {
    struct slave_data *data = device->private;
    assert(data);

    mutex_lock(&data->device->lock);
    data->ref_count++;
    mutex_unlock(&data->device->lock);
}

static ssize_t slave_read(struct fs_device *device, off_t offset, void *buf, size_t _len, bool non_blocking) {
    assert(offset == 0);

    struct slave_data *data = device->private;
    if (get_current_task()->process->pgid != data->pgid) {
#ifdef PTMX_SIGNAL_DEBUG
        debug_log("Sending SIGTTIN: [ %d, %d, %d ]\n", data->pgid, get_current_task()->process->pgid, get_current_task()->process->pid);
#endif /* PTMX_SIGNAL_DEBUG */
        signal_process_group(get_current_task()->process->pgid, SIGTTIN);
    }

    mutex_lock(&data->device->lock);
again:;
    size_t len = MIN(_len, ring_buffer_size(&data->input_buffer));
    if (len == 0) {
        if (non_blocking) {
            return 0;
        }

        if (!(data->config.c_lflag & ICANON) && data->config.c_cc[VTIME] != 0) {
            time_t timeout_ms = data->config.c_cc[VTIME] * 100;
            struct timespec timeout = { .tv_sec = timeout_ms / 1000, .tv_nsec = (timeout_ms % 1000) * 1000000 };
            int ret = dev_poll_wait(data->device, POLLIN, &timeout);
            if (ret) {
                return ret;
            }

            if (timeout.tv_sec == 0 && timeout.tv_nsec == 0) {
                return 0;
            }
        } else {
            int ret = dev_poll_wait(data->device, POLLIN, NULL);
            if (ret) {
                return ret;
            }
        }
        goto again;
    }

    if (data->config.c_lflag & ICANON) {
        char c = ring_buffer_read_byte(&data->input_buffer);
        if (c == data->config.c_cc[VEOF]) {
            len = 0;
        } else {
            ((char *) buf)[0] = c;
            for (size_t i = 1; i < len; i++) {
                char c = ring_buffer_read_byte(&data->input_buffer);
                if (c == data->config.c_cc[VEOF]) {
                    len = i;
                    break;
                }
                ((char *) buf)[i] = c;
            }
        }
    } else {
        ring_buffer_user_read(&data->input_buffer, buf, len);
    }

    struct master_data *mdata = masters[data->index]->private;
    fs_set_state_bit(&data->device->file_state, POLLIN, !ring_buffer_empty(&data->input_buffer));

    mutex_unlock(&data->device->lock);

    mutex_lock(&mdata->device->lock);
    fs_trigger_state(&mdata->device->file_state, POLLOUT);
    mutex_unlock(&mdata->device->lock);

    return (ssize_t) len;
}

static ssize_t slave_write(struct fs_device *device, off_t offset, const void *buf, size_t len, bool non_blocking) {
    assert(offset == 0);
    (void) non_blocking;

    if (len == 0) {
        return 0;
    }

    struct slave_data *data = device->private;
    if (get_current_task()->process->pgid != data->pgid && (data->config.c_lflag & TOSTOP)) {
#ifdef PTMX_SIGNAL_DEBUG
        debug_log("Sending SIGTTOU: [ %d, %d, %d ]\n", data->pgid, get_current_task()->process->pgid, get_current_task()->process->pid);
#endif /* PTMX_SIGNAL_DEBUG */
        signal_process_group(get_current_task()->process->pgid, SIGTTOU);
    }

    size_t save_len = len;
    if (data->config.c_oflag & OPOST) {
        for (size_t i = 0; i < save_len; i++) {
            if (((const char *) buf)[i] == '\n') {
                len++;
            }
        }
    }

    struct master_data *mdata = masters[data->index]->private;
    mutex_lock(&mdata->device->lock);

    size_t raw_buffer_index = 0;
    size_t bytes_written = 0;
    while (raw_buffer_index < save_len) {
        size_t space_available = ring_buffer_space(&mdata->output_buffer);

        // NOTE: reserve a trailing byte so that OPOST can always add its '\r' character.
        if (space_available) {
            space_available--;
        }

        if (!space_available) {
            mutex_unlock(&mdata->device->lock);
            mutex_lock(&data->device->lock);
            int ret = dev_poll_wait(data->device, POLLOUT, NULL);
            if (ret) {
                return ret;
            }
            mutex_unlock(&data->device->lock);
            mutex_lock(&mdata->device->lock);
            continue;
        }

        size_t amount_to_write = MIN(space_available, len - bytes_written);
        assert(amount_to_write != 0);

        if (data->config.c_oflag & OPOST) {
            for (size_t i = 0; i < amount_to_write; i++) {
                char character_to_write = ((char *) buf)[raw_buffer_index++];
                if (character_to_write == '\n') {
                    ring_buffer_write_byte(&mdata->output_buffer, '\r');
                    ring_buffer_write_byte(&mdata->output_buffer, '\n');
                    i++;
                    bytes_written += 2;
                    continue;
                }

                ring_buffer_write_byte(&mdata->output_buffer, character_to_write);
                bytes_written++;
            }
        } else {
            ring_buffer_user_write(&mdata->output_buffer, buf + raw_buffer_index, amount_to_write);
            raw_buffer_index += amount_to_write;
            bytes_written += amount_to_write;
        }

        fs_trigger_state(&mdata->device->file_state, POLLIN);

        mutex_unlock(&mdata->device->lock);
        mutex_lock(&data->device->lock);
        fs_set_state_bit(&data->device->file_state, POLLOUT, !ring_buffer_full(&mdata->output_buffer));
        mutex_unlock(&data->device->lock);
        mutex_lock(&mdata->device->lock);
    }

    mutex_unlock(&mdata->device->lock);
    return (ssize_t) save_len;
}

static int slave_close(struct fs_device *device) {
    struct slave_data *data = device->private;
    assert(data);

    mutex_lock(&data->device->lock);
    data->ref_count--;
    if (data->ref_count <= 0) {
        // data->device->lock will be unlocked in remove callback
        dev_unregister(device);
        return 0;
    }

    mutex_unlock(&data->device->lock);
    return 0;
}

static void slave_add(struct fs_device *device) {
    struct slave_data *data = calloc(1, sizeof(struct slave_data));
    data->ref_count = 1; // For the master

    data->cols = VGA_WIDTH;
    data->rows = VGA_HEIGHT;
    data->pgid = get_current_task()->process->pgid;
    data->input_enabled = data->output_enabled = true;

    memcpy(&data->config, &default_termios, sizeof(struct termios));
    ptmx_log_termios(&data->config);

    for (int i = 0; i < PTMX_MAX; i++) {
        if (device == slaves[i]) {
            data->index = i;
            break;
        }
    }

    init_file_state(&device->file_state, false, false);
    init_ring_buffer(&data->input_buffer, PTMX_BUFFER_MAX);

    device->private = data;
    data->device = device;
}

static void slave_remove(struct fs_device *device) {
    struct slave_data *data = device->private;
    assert(data);

    mutex_unlock(&data->device->lock);

    slaves[data->index] = NULL;

    kill_ring_buffer(&data->input_buffer);

    debug_log("Removing slave tty: [ %d ]\n", data->index);

    int index = data->index;
    free(data);

    char path[32];
    snprintf(path, sizeof(path) - 1, "/dev/tty%d", index);
    assert(fs_unlink(path, true) == 0);
}

static int slave_ioctl(struct fs_device *device, unsigned long request, void *argp) {
    if (request == TISATTY) {
        return 0;
    }

    struct slave_data *data = device->private;

    switch (request) {
        case TCSETSF:
        case TCSETSW:
        case TCSETS:
            if (get_current_task()->process->pgid != data->pgid) {
#ifdef PTMX_SIGNAL_DEBUG
                debug_log("Sending SIGTTOU: [ %d, %d, %d ]\n", data->pgid, get_current_task()->process->pgid,
                          get_current_task()->process->pid);
#endif /* PTMX_SIGNAL_DEBUG */
                signal_process_group(get_current_task()->process->pgid, SIGTTOU);
            }
            break;
        default:
            break;
    }

    struct fs_device *master = masters[data->index];
    struct master_data *mdata = master->private;

    switch (request) {
        case TIOCSWINSZ: {
            struct winsize *w = argp;
            mutex_lock(&data->device->lock);
            data->rows = w->ws_row;
            data->cols = w->ws_col;
            mutex_unlock(&data->device->lock);
            return 0;
        }
        case TIOCGWINSZ: {
            struct winsize *w = argp;
            w->ws_row = data->rows;
            w->ws_col = data->cols;
            return 0;
        }
        case TIOCGPGRP: {
            return data->pgid;
        }
        case TIOCSPGRP: {
            mutex_lock(&data->device->lock);
            data->pgid = *((pid_t *) argp);
            mutex_unlock(&data->device->lock);
            return 0;
        }
        case TCGETS: {
            memcpy(argp, &data->config, sizeof(struct termios));
            return 0;
        }
        case TCSETSF: {
            mutex_lock(&mdata->device->lock);
            free(mdata->input_buffer);
            mdata->input_buffer = NULL;
            mdata->input_buffer_length = 0;
            mdata->input_buffer_max = 0;
            mutex_unlock(&mdata->device->lock);
        } // Fall through
        case TCSETSW: {
            // Should flush output
        } // Fall through
        case TCSETS: {
            mutex_lock(&data->device->lock);
            memcpy(&data->config, argp, sizeof(struct termios));
            ptmx_log_termios(&data->config);
            mutex_unlock(&data->device->lock);
            return 0;
        }
        case TGETNUM: {
            int *i = argp;
            *i = data->index;
            return 0;
        }
        case TIOCSCTTY: {
            int ret = 0;
            mutex_lock(&data->device->lock);
            struct process *current = get_current_task()->process;
            if (current->sid != current->pid || current->pgid != current->pid) {
                ret = -EPERM;
                goto finish_slave_ioctl_tiosctty;
            }

            current->tty = data->index;
            data->pgid = current->pid;
            data->sid = current->sid;

        finish_slave_ioctl_tiosctty:
            mutex_unlock(&data->device->lock);
            return ret;
        }
        case TIOCNOTTY: {
            get_current_task()->process->tty = -1;
            return 0;
        }
        case TCIOFFI: {
            mutex_lock(&data->device->lock);
            data->input_enabled = false;
            mutex_unlock(&data->device->lock);
            return 0;
        }
        case TCOOFFI: {
            mutex_lock(&data->device->lock);
            data->output_enabled = false;
            mutex_unlock(&data->device->lock);
            return 0;
        }
        case TCIONI: {
            mutex_lock(&data->device->lock);
            data->input_enabled = true;
            mutex_unlock(&data->device->lock);
            return 0;
        }
        case TCOONI: {
            mutex_lock(&data->device->lock);
            data->output_enabled = true;
            mutex_unlock(&data->device->lock);
            return 0;
        }
        case TIOCGSID: {
            mutex_lock(&data->device->lock);
            pid_t ret = data->sid;
            mutex_unlock(&data->device->lock);
            return ret;
        }
        default: {
            return -ENOTTY;
        }
    }
}

static struct fs_device_ops slave_ops = {
    .read = slave_read,
    .write = slave_write,
    .close = slave_close,
    .add = slave_add,
    .remove = slave_remove,
    .ioctl = slave_ioctl,
    .on_open = slave_on_open,
};

static void master_on_open(struct fs_device *device) {
    device->cannot_open = true;
}

static ssize_t master_read(struct fs_device *device, off_t offset, void *buf, size_t _len, bool non_blocking) {
    assert(offset == 0);
    (void) non_blocking;

    struct master_data *data = device->private;
    struct slave_data *sdata = slaves[data->index]->private;

    mutex_lock(&data->device->lock);

    size_t len = MIN(_len, ring_buffer_size(&data->output_buffer));
    if (len == 0) {
        mutex_unlock(&data->device->lock);
        return 0;
    }

    ring_buffer_user_read(&data->output_buffer, buf, len);

    fs_set_state_bit(&data->device->file_state, POLLIN, !ring_buffer_empty(&data->output_buffer));
    mutex_unlock(&data->device->lock);

    mutex_lock(&sdata->device->lock);
    if (ring_buffer_space(&data->output_buffer) > 1) {
        fs_trigger_state(&sdata->device->file_state, POLLOUT);
    }
    mutex_unlock(&sdata->device->lock);

    return (ssize_t) len;
}

static void tty_do_echo(struct master_data *data, struct slave_data *sdata, char c) {
    if (sdata->config.c_lflag & ECHO) {
        mutex_unlock(&data->device->lock);
        slave_write(slaves[data->index], 0, &c, 1, false);
        mutex_lock(&data->device->lock);
    }
}

static bool tty_do_signals(struct slave_data *sdata, char c) {
    if (!(sdata->config.c_lflag & ISIG)) {
        return false;
    }

    if (c == sdata->config.c_cc[VINTR]) {
        signal_process_group(sdata->pgid, SIGINT);
    } else if (c == sdata->config.c_cc[VSUSP]) {
        signal_process_group(sdata->pgid, SIGTSTP);
    } else if (c == sdata->config.c_cc[VQUIT]) {
        signal_process_group(sdata->pgid, SIGQUIT);
    } else {
        return false;
    }

    return true;
}

static ssize_t master_write(struct fs_device *device, off_t offset, const void *buf, size_t len, bool non_blocking) {
    assert(offset == 0);
    (void) non_blocking;

    if (len == 0) {
        return 0;
    }

    struct master_data *data = device->private;
    struct slave_data *sdata = slaves[data->index]->private;

    mutex_lock(&data->device->lock);

    for (size_t i = 0; i < len; i++) {
        if ((sdata->config.c_lflag & ICANON) && data->input_buffer_length >= data->input_buffer_max) {
            data->input_buffer_max += PTMX_BUFFER_MAX;
            data->input_buffer = realloc(data->input_buffer, data->input_buffer_max);
        }

        char c = ((const char *) buf)[i];

        if (c == '\r' && sdata->config.c_iflag & ICRNL) {
            c = '\n';
        }

        if (c == sdata->config.c_cc[VERASE] && (sdata->config.c_lflag & ICANON)) {
            if (data->input_buffer_length == 0) {
                continue;
            }

            tty_do_echo(data, sdata, c);
            data->input_buffer[--data->input_buffer_length] = '\0';
            continue;
        }

        if ((c == sdata->config.c_cc[VEOL] || c == sdata->config.c_cc[VEOF]) && (sdata->config.c_lflag & ICANON)) {
            if (c == sdata->config.c_cc[VEOL]) {
                tty_do_echo(data, sdata, c);
            }
            data->input_buffer[data->input_buffer_length++] = c;

            mutex_lock(&sdata->device->lock);

            size_t can_write = MIN(data->input_buffer_length, ring_buffer_space(&sdata->input_buffer));
            ring_buffer_write(&sdata->input_buffer, data->input_buffer, can_write);

            free(data->input_buffer);
            data->input_buffer = NULL;
            data->input_buffer_length = data->input_buffer_max = 0;

#ifdef PTMX_BLOCKING_DEBUG
            debug_log("Making slave readable: [ %d ]\n", sdata->index);
#endif /* PTMX_BLOCKING_DEBUG */

            fs_trigger_state(&sdata->device->file_state, POLLIN);

            mutex_unlock(&sdata->device->lock);
            continue;
        }

        tty_do_echo(data, sdata, c);

        if (tty_do_signals(sdata, c)) {
            free(data->input_buffer);
            data->input_buffer = NULL;
            data->input_buffer_length = data->input_buffer_max = 0;
            i = 0;
            continue;
        }

        if (!(sdata->config.c_lflag & ICANON)) {
            mutex_lock(&sdata->device->lock);

            if (!ring_buffer_full(&sdata->input_buffer)) {
                ring_buffer_write_byte(&sdata->input_buffer, c);
                fs_trigger_state(&sdata->device->file_state, POLLIN);
            }

            mutex_unlock(&sdata->device->lock);
            continue;
        }

        data->input_buffer[data->input_buffer_length++] = c;
    }

    mutex_unlock(&data->device->lock);
    return (ssize_t) len;
}

static int master_close(struct fs_device *device) {
    dev_unregister(device);
    return 0;
}

static void master_add(struct fs_device *device) {
    struct master_data *data = calloc(1, sizeof(struct master_data));
    for (int i = 0; i < PTMX_MAX; i++) {
        if (device == masters[i]) {
            data->index = i;
            break;
        }
    }

    init_file_state(&device->file_state, false, false);

    init_ring_buffer(&data->output_buffer, PTMX_BUFFER_MAX);

    device->private = data;
    data->device = device;
}

static void master_remove(struct fs_device *device) {
    struct master_data *data = device->private;
    assert(data);

    debug_log("Removing master tty: [ %d ]\n", data->index);

    slave_close(slaves[data->index]);

    masters[data->index] = NULL;

    free(data->input_buffer);
    kill_ring_buffer(&data->output_buffer);

    int index = data->index;
    free(data);

    char path[32];
    snprintf(path, sizeof(path) - 1, "/dev/mtty%d", index);
    assert(fs_unlink(path, true) == 0);
}

static int master_ioctl(struct fs_device *device, unsigned long request, void *argp) {
    // We're not a real termial, just a controller of one
    if (request == TISATTY) {
        return -ENOTTY;
    }

    struct master_data *data = device->private;
    struct fs_device *slave = slaves[data->index];
    return slave_ioctl(slave, request, argp);
}

static struct fs_device_ops master_ops = {
    .read = master_read,
    .write = master_write,
    .close = master_close,
    .add = master_add,
    .remove = master_remove,
    .ioctl = master_ioctl,
    .on_open = master_on_open,
};

static struct file *ptmx_open(struct fs_device *device, int flags, int *error) {
    (void) device;

    mutex_lock(&lock);
    for (int i = 0; i < PTMX_MAX; i++) {
        if (slaves[i] == NULL && masters[i] == NULL) {
            slaves[i] = calloc(1, sizeof(struct fs_device));
            mutex_unlock(&lock);

            slaves[i]->device_number = 0x00300 + i;
            slaves[i]->ops = &slave_ops;
            slaves[i]->mode = S_IFCHR | 0666;
            slaves[i]->private = NULL;
            init_mutex(&slaves[i]->lock);
            snprintf(slaves[i]->name, sizeof(slaves[i]->name) - 1, "tty%d", i);

            struct fs_device *master = calloc(1, sizeof(struct fs_device));
            master->device_number = 0x00400 + i;
            master->ops = &master_ops;
            master->mode = S_IFCHR | 0666;
            master->private = NULL;
            init_mutex(&master->lock);
            masters[i] = master;
            snprintf(master->name, sizeof(master->name) - 1, "mtty%d", i);

            struct inode *master_inode = dev_register(masters[i]);
            dev_register(slaves[i]);

            return dev_open(master_inode, flags, error);
        }
    }

    mutex_unlock(&lock);
    *error = -ENOMEM;
    return NULL;
}

struct fs_device_ops ptmx_ops = { .open = ptmx_open };

static struct file *tty_open(struct fs_device *device, int flags, int *error) {
    (void) device;

    int tty_num = get_current_task()->process->tty;
    if (tty_num == -1) {
        *error = -ENOENT;
        return NULL;
    }

    char path[20] = { 0 };
    snprintf(path, 19, "/dev/tty%d", tty_num);
    debug_log("Redirecting /dev/tty to [ %s ]\n", path);
    return fs_openat(NULL, path, flags, 0, error);
}

struct fs_device_ops tty_ops = { .open = tty_open };

void init_ptmx() {
    struct fs_device *ptmx_device = calloc(1, sizeof(struct fs_device));
    ptmx_device->device_number = 0x00201;
    ptmx_device->ops = &ptmx_ops;
    ptmx_device->private = NULL;
    ptmx_device->mode = S_IFCHR | 0666;
    init_mutex(&ptmx_device->lock);
    strcpy(ptmx_device->name, "ptmx");

    dev_register(ptmx_device);

    struct fs_device *tty_device = calloc(1, sizeof(struct fs_device));
    tty_device->device_number = 0x00202;
    tty_device->ops = &tty_ops;
    tty_device->private = NULL;
    tty_device->mode = S_IFCHR | 0666;
    init_mutex(&ptmx_device->lock);
    strcpy(tty_device->name, "tty");

    dev_register(tty_device);
}
