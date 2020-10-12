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
#include <kernel/hal/timer.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>
#include <kernel/util/mutex.h>

// #define PTMX_BLOCKING_DEBUG
// #define PTMX_SIGNAL_DEBUG

#define PTMX_MAX 16

#define TTY_BUF_START 4096

#define CONTROL_MASK   0x1F
#define CONTROL_KEY(c) ((c) &CONTROL_MASK)

static struct termios default_termios = { ICRNL | IXON,
                                          OPOST,
                                          CS8,
                                          ECHO | ICANON | IEXTEN | ISIG,
                                          { CONTROL_KEY('d'), '\n', '\b', CONTROL_KEY('c'), '\025', 1, CONTROL_KEY('\\'), CONTROL_KEY('q'),
                                            CONTROL_KEY('s'), CONTROL_KEY('z'), 0 } };

static struct fs_device *slaves[PTMX_MAX] = { 0 };
static struct fs_device *masters[PTMX_MAX] = { 0 };
static mutex_t lock = MUTEX_INITIALIZER;

static void slave_on_open(struct fs_device *device) {
    struct slave_data *data = device->private;
    assert(data);

    mutex_lock(&data->device->lock);
    data->ref_count++;
    mutex_unlock(&data->device->lock);
}

static ssize_t slave_read(struct fs_device *device, off_t offset, void *buf, size_t len, bool non_blocking) {
    assert(offset == 0);

    struct slave_data *data = device->private;
    if (get_current_task()->process->pgid != data->pgid) {
#ifdef PTMX_SIGNAL_DEBUG
        debug_log("Sending SIGTTIN: [ %d, %d, %d ]\n", data->pgid, get_current_task()->process->pgid, get_current_task()->process->pid);
#endif /* PTMX_SIGNAL_DEBUG */
        signal_process_group(get_current_task()->process->pgid, SIGTTIN);
    }

    mutex_lock(&data->device->lock);
    if (data->input_buffer == NULL) {
        while (data->messages == NULL) {
            mutex_unlock(&data->device->lock);

            if (non_blocking) {
                return 0;
            }

#ifdef PTMX_BLOCKING_DEBUG
            debug_log("Blocking on slave read: [ %d ]\n", data->index);
#endif /* PTMX_BLOCKING_DEBUG */

            if (!(data->config.c_lflag & ICANON) && data->config.c_cc[VTIME] != 0) {
                time_t end_time_ms = data->config.c_cc[VTIME] * 100;
                struct timespec end_time = { .tv_sec = end_time_ms / 1000, .tv_nsec = (end_time_ms % 1000) * 1000000 };
                int ret = proc_block_until_device_is_readable_or_timeout(get_current_task(), device, end_time);
                if (ret) {
                    return ret;
                }

                if (time_compare(time_read_clock(CLOCK_MONOTONIC), end_time) >= 0) {
                    return 0;
                }

                mutex_lock(&data->device->lock);
                // This is because multiple processes could be waiting for ptmx input, meaning
                // a different one could have consumed the message, and since there is a timeout
                // we don't want to block again.
                if (data->messages == NULL) {
                    mutex_unlock(&data->device->lock);
                }
                break;
            } else {
                int ret = proc_block_until_device_is_readable(get_current_task(), device);
                if (ret) {
                    return ret;
                }
            }
            mutex_lock(&data->device->lock);
        }

        struct tty_buffer_message *message = data->messages;
        data->messages = message == message->next ? NULL : message->next;
        remque(message);

        data->input_buffer = malloc(message->len);
        memcpy(data->input_buffer, message->buf, message->len);
        data->input_buffer_length = data->input_buffer_max = message->len;

        free(message->buf);
        free(message);
    }

    size_t to_copy = MIN(len, data->input_buffer_length - data->input_buffer_index);
    memcpy(buf, data->input_buffer + data->input_buffer_index, to_copy);
    data->input_buffer_index += to_copy;

    if (data->input_buffer_index >= data->input_buffer_length) {
        free(data->input_buffer);
        data->input_buffer = NULL;
        data->input_buffer_index = data->input_buffer_length = data->input_buffer_max = 0;

        // Clear the readable flag once we've consumed to input_buffer
        // and there is no messages
        if (data->messages == NULL) {
#ifdef PTMX_BLOCKING_DEBUG
            debug_log("Setting readable flag on slave to false: [ %d ]\n", data->index);
#endif /* PTMX_BLOCKING_DEBUG */
            device->readable = false;
        }
    }

    mutex_unlock(&data->device->lock);

    return (ssize_t) to_copy;
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

    struct tty_buffer_message *message = mdata->messages;
slave_write_again:
    if (message == NULL) {
        message = calloc(1, sizeof(struct tty_buffer_message));
        message->max = MAX(TTY_BUF_MAX_START, len);
        message->buf = malloc(message->max);
        message->len = 0;
        message->prev = message->next = message;
        mdata->messages = message;
    } else {
        if (message->max < message->len + len) {
            mdata->device->writeable = false;
            while (mdata->messages != NULL) {
                mutex_unlock(&mdata->device->lock);
#ifdef PTMX_BLOCKING_DEBUG
                debug_log("Blocking until master is writable: [ %d ]\n", mdata->index);
#endif /* PTMX_BLOCKING_DEBUG */
                int ret = proc_block_until_device_is_writeable(get_current_task(), mdata->device);
                if (ret) {
                    return ret;
                }
                mutex_lock(&mdata->device->lock);
            }

            message = mdata->messages;
            goto slave_write_again;
        }
    }

    size_t buf_index = 0;
    for (size_t i = message->len; i < message->len + len; i++, buf_index++) {
        if ((data->config.c_oflag & OPOST) && ((const char *) buf)[buf_index] == '\n') {
            message->buf[i++] = '\r';
        }
        message->buf[i] = ((const char *) buf)[buf_index];
    }

    message->len += len;

    // This is now readable since we wrote to id
#ifdef PTMX_BLOCKING_DEBUG
    debug_log("Setting master to readable: [ %d ]\n", mdata->index);
#endif /* PTMX_BLOCKING_DEBUG */
    mdata->device->readable = true;

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

    for (int i = 0; i < PTMX_MAX; i++) {
        if (device == slaves[i]) {
            data->index = i;
            break;
        }
    }

    device->readable = false;

    device->private = data;
    data->device = device;
}

static void slave_remove(struct fs_device *device) {
    struct slave_data *data = device->private;
    assert(data);

    mutex_unlock(&data->device->lock);

    slaves[data->index] = NULL;

    free(data->input_buffer);

    debug_log("Removing slave tty: [ %d ]\n", data->index);

    while (data->messages) {
        struct tty_buffer_message *m = data->messages->next == data->messages ? NULL : data->messages->next;
        remque(data->messages);
        free(data->messages->buf);
        free(data->messages);
        data->messages = m;
    }

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
            mutex_unlock(&data->device->lock);
            return 0;
        }
        case TGETNUM: {
            int *i = argp;
            *i = data->index;
            return 0;
        }
        case TIOSCTTY: {
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

static struct fs_device_ops slave_ops = { .read = slave_read,
                                          .write = slave_write,
                                          .close = slave_close,
                                          .add = slave_add,
                                          .remove = slave_remove,
                                          .ioctl = slave_ioctl,
                                          .on_open = slave_on_open };

static void master_on_open(struct fs_device *device) {
    device->cannot_open = true;
}

static ssize_t master_read(struct fs_device *device, off_t offset, void *buf, size_t len, bool non_blocking) {
    assert(offset == 0);
    (void) non_blocking;

    struct master_data *data = device->private;

    mutex_lock(&data->device->lock);
    if (data->output_buffer == NULL) {
        if (data->messages == NULL) {
            mutex_unlock(&data->device->lock);
            return 0;
        }

        struct tty_buffer_message *message = data->messages;
        assert(message);
        assert(message->buf);

        data->messages = message == message->next ? NULL : message->next;
        remque(message);

        data->output_buffer = malloc(message->len);
        assert(data->output_buffer);
        data->output_buffer_length = data->output_buffer_max = message->len;
        memcpy(data->output_buffer, message->buf, data->output_buffer_length);

        free(message->buf);
        free(message);
    }

    size_t to_read = MIN(len, data->output_buffer_length - data->output_buffer_index);
    memcpy(buf, data->output_buffer + data->output_buffer_index, to_read);
    data->output_buffer_index += to_read;

    if (data->output_buffer_index >= data->output_buffer_length) {
        free(data->output_buffer);
        data->output_buffer = NULL;
        data->output_buffer_index = data->output_buffer_length = data->output_buffer_max = 0;

        // Reset readable/writable flags since we consumed to buffer
        if (data->messages == NULL) {
#ifdef PTMX_BLOCKING_DEBUG
            debug_log("Resetting master flags: [ %d ]\n", data->index);
#endif /* PTMX_BLOCKING_DEBUG */
            device->writeable = true;
            device->readable = false;
        }
    }

    mutex_unlock(&data->device->lock);

    return (ssize_t) to_read;
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
            data->input_buffer_max += TTY_BUF_MAX_START;
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
                data->input_buffer[data->input_buffer_length++] = c;
            }

            mutex_lock(&sdata->device->lock);

            struct tty_buffer_message *message = calloc(1, sizeof(struct tty_buffer_message));
            message->len = message->max = data->input_buffer_length;
            message->buf = malloc(message->len);
            memcpy(message->buf, data->input_buffer, message->len);

            if (sdata->messages == NULL) {
                sdata->messages = message->next = message->prev = message;
            } else {
                insque(message, sdata->messages->prev);
            }

            free(data->input_buffer);
            data->input_buffer = NULL;
            data->input_buffer_length = data->input_buffer_max = 0;

#ifdef PTMX_BLOCKING_DEBUG
            debug_log("Making slave readable: [ %d ]\n", sdata->index);
#endif /* PTMX_BLOCKING_DEBUG */

            // The slave is readable now that we wrote to it.
            sdata->device->readable = true;

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

            if (sdata->messages == NULL) {
                sdata->messages = calloc(1, sizeof(struct tty_buffer_message));
                sdata->messages->buf = malloc(TTY_BUF_MAX_START);
                sdata->messages->len = 1;
                sdata->messages->max = TTY_BUF_MAX_START;
                sdata->messages->buf[0] = c;
                sdata->messages->prev = sdata->messages->next = sdata->messages;
            } else {
                struct tty_buffer_message *m = sdata->messages->prev;

                if (m->len >= m->max) {
                    m->max += TTY_BUF_MAX_START;
                    m->buf = realloc(m->buf, m->max);
                }

                m->buf[m->len++] = c;
            }

#ifdef PTMX_BLOCKING_DEBUG
            debug_log("Making slave readable: [ %d ]\n", sdata->index);
#endif /* PTMX_BLOCKING_DEBUG */

            // The slave is readable now that we wrote to it.
            sdata->device->readable = true;

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

    device->readable = false;

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
    free(data->output_buffer);

    while (data->messages) {
        struct tty_buffer_message *m = data->messages->next == data->messages ? NULL : data->messages->next;
        remque(data->messages);
        free(data->messages->buf);
        free(data->messages);
        data->messages = m;
    }

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

static struct fs_device_ops master_ops = { .read = master_read,
                                           .write = master_write,
                                           .close = master_close,
                                           .add = master_add,
                                           .remove = master_remove,
                                           .ioctl = master_ioctl,
                                           .on_open = master_on_open };

static int noop_unlink(struct tnode *tnode) {
    (void) tnode;
    return 0;
}

static struct inode_operations empty_ops = { .unlink = noop_unlink };

static struct file *ptmx_open(struct fs_device *device, int flags, int *error) {
    (void) device;

    mutex_lock(&lock);
    for (int i = 0; i < PTMX_MAX; i++) {
        if (slaves[i] == NULL && masters[i] == NULL) {
            slaves[i] = calloc(1, sizeof(struct fs_device));
            mutex_unlock(&lock);

            slaves[i]->device_number = 0x00300 + i;
            slaves[i]->ops = &slave_ops;
            slaves[i]->type = S_IFCHR;
            slaves[i]->private = NULL;

            struct fs_device *master = calloc(1, sizeof(struct fs_device));
            master->device_number = 0x00400 + i;
            master->ops = &master_ops;
            master->type = S_IFCHR;
            master->private = NULL;
            masters[i] = master;

            dev_register(masters[i]);
            dev_register(slaves[i]);

            char master_name[16];
            size_t master_length = snprintf(master_name, sizeof(master_name) - 1, "mtty%d", i);

            struct tnode *tnode;
            assert(iname("/dev", 0, &tnode) == 0);

            struct inode *master_inode = fs_create_inode_without_sb(tnode->inode->fsid, i, get_current_task()->process->euid,
                                                                    get_current_task()->process->egid, S_IFCHR | 0777, 0, &empty_ops, NULL);
            master_inode->device_id = master->device_number;

            char slave_name[16];
            size_t slave_length = snprintf(slave_name, sizeof(slave_name) - 1, "tty%d", i);

            struct inode *slave_inode = fs_create_inode_without_sb(tnode->inode->fsid, PTMX_MAX + i, get_current_task()->process->euid,
                                                                   get_current_task()->process->egid, S_IFCHR | 0777, 0, &empty_ops, NULL);
            slave_inode->device_id = slaves[i]->device_number;

            fs_put_dirent_cache(tnode->inode->dirent_cache, master_inode, master_name, master_length);
            fs_put_dirent_cache(tnode->inode->dirent_cache, slave_inode, slave_name, slave_length);
            drop_tnode(tnode);

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
    ptmx_device->type = S_IFCHR;

    dev_register(ptmx_device);

    struct fs_device *tty_device = calloc(1, sizeof(struct fs_device));
    tty_device->device_number = 0x00202;
    tty_device->ops = &tty_ops;
    tty_device->private = NULL;
    tty_device->type = S_IFCHR;

    dev_register(tty_device);
}
