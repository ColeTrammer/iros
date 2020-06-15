#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/vfs.h>
#include <kernel/proc/task.h>

static ssize_t dev_null_read(struct device *device, off_t offset, void *buffer, size_t len) {
    (void) device;
    (void) offset;
    (void) buffer;
    (void) len;

    return 0;
}

static ssize_t dev_zero_read(struct device *device, off_t offset, void *buffer, size_t len) {
    (void) device;
    (void) offset;

    memset(buffer, 0, len);
    return len;
}

static ssize_t dev_ignore_write(struct device *device, off_t offset, const void *buffer, size_t len) {
    (void) device;
    (void) offset;
    (void) buffer;

    return len;
}

static struct file *fake_symlink(struct file *file, int flags, int *error) {
    if (file == NULL) {
        *error = ENOENT;
        return NULL;
    }

    struct tnode *tnode = file->tnode;

    char *path = get_tnode_path(tnode);
    debug_log("to: [ %s ]\n", path);

    struct file *ret = fs_openat(NULL, path, flags, 0, error);

    free(path);
    return ret;
}

static struct file *stdin_open(struct device *device, int flags, int *error) {
    (void) device;

    struct task *task = get_current_task();
    struct file *file = task->process->files[STDIN_FILENO].file;

    debug_log("redirecting: [ %s ]\n", "/dev/stdin");
    return fake_symlink(file, flags, error);
}

static struct file *stdout_open(struct device *device, int flags, int *error) {
    (void) device;

    struct task *task = get_current_task();
    struct file *file = task->process->files[STDOUT_FILENO].file;

    debug_log("redirecting: [ %s ]\n", "/dev/stdout");
    return fake_symlink(file, flags, error);
}

static struct file *stderr_open(struct device *device, int flags, int *error) {
    (void) device;

    struct task *task = get_current_task();
    struct file *file = task->process->files[STDERR_FILENO].file;

    debug_log("redirecting: [ %s ]\n", "/dev/stderr");
    return fake_symlink(file, flags, error);
}

static ssize_t full_read(struct device *device, off_t offset, void *buf, size_t n) {
    (void) device;
    (void) offset;
    (void) buf;
    (void) n;

    return -ENOSPC;
}

static ssize_t full_write(struct device *device, off_t offset, const void *buf, size_t n) {
    (void) device;
    (void) offset;
    (void) buf;
    (void) n;

    return -ENOSPC;
}

static const char *fd_path = "/proc/self/fd";

static ssize_t fd_read(struct device *device, off_t offset, void *buf, size_t n) {
    (void) device;

    return fs_do_read(buf, offset, n, fd_path, strlen(fd_path));
}

static int fd_read_all(struct device *device, void *buf) {
    (void) device;

    size_t to_read = strlen(fd_path);
    return fs_do_read(buf, 0, to_read, fd_path, to_read);
}

static void fd_add(struct device *device) {
    device->inode->size = strlen(fd_path);
}

static struct device_ops dev_null_ops = { .read = &dev_null_read, .write = &dev_ignore_write };

static struct device dev_null = { .device_number = 0x31, .type = S_IFCHR, .ops = &dev_null_ops, .lock = SPINLOCK_INITIALIZER };

static struct device_ops dev_zero_ops = { .read = &dev_zero_read, .write = &dev_ignore_write };

static struct device dev_zero = { .device_number = 0x32, .type = S_IFCHR, .ops = &dev_zero_ops, .lock = SPINLOCK_INITIALIZER };

static struct device_ops dev_stdin_ops = { .open = &stdin_open };

static struct device_ops dev_stdout_ops = { .open = &stdout_open };

static struct device_ops dev_stderr_ops = { .open = &stderr_open };

static struct device_ops dev_fd_ops = { .read = &fd_read, .add = &fd_add, .read_all = &fd_read_all };

static struct device dev_stdin = { .device_number = 0x33, .type = S_IFCHR, .ops = &dev_stdin_ops, .lock = SPINLOCK_INITIALIZER };

static struct device dev_stdout = { .device_number = 0x34, .type = S_IFCHR, .ops = &dev_stdout_ops, .lock = SPINLOCK_INITIALIZER };

static struct device dev_stderr = { .device_number = 0x35, .type = S_IFCHR, .ops = &dev_stderr_ops, .lock = SPINLOCK_INITIALIZER };

static struct device_ops dev_full_ops = { .read = &full_read, .write = &full_write };

static struct device dev_full = { .device_number = 0x36, .type = S_IFCHR, .ops = &dev_full_ops, .lock = SPINLOCK_INITIALIZER };

static struct device dev_fd = { .device_number = 0x37, .type = S_IFLNK, .ops = &dev_fd_ops, .lock = SPINLOCK_INITIALIZER };

void init_virtual_devices() {
    dev_register(&dev_null);
    dev_register(&dev_zero);
    dev_register(&dev_stdin);
    dev_register(&dev_stdout);
    dev_register(&dev_stderr);
    dev_register(&dev_full);
    dev_register(&dev_fd);
}
