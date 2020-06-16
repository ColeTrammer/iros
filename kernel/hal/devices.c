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

static struct device_ops dev_null_ops = { .read = &dev_null_read, .write = &dev_ignore_write };

static struct device dev_null = { .device_number = 0x31, .type = S_IFCHR, .ops = &dev_null_ops, .lock = SPINLOCK_INITIALIZER };

static struct device_ops dev_zero_ops = { .read = &dev_zero_read, .write = &dev_ignore_write };

static struct device dev_zero = { .device_number = 0x32, .type = S_IFCHR, .ops = &dev_zero_ops, .lock = SPINLOCK_INITIALIZER };

static struct device_ops dev_full_ops = { .read = &full_read, .write = &full_write };

static struct device dev_full = { .device_number = 0x36, .type = S_IFCHR, .ops = &dev_full_ops, .lock = SPINLOCK_INITIALIZER };

void init_virtual_devices() {
    dev_register(&dev_null);
    dev_register(&dev_zero);
    dev_register(&dev_full);
}
