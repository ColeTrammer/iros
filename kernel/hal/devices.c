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
#include <kernel/hal/ptmx.h>
#include <kernel/proc/task.h>
#include <kernel/util/random.h>

static ssize_t dev_null_read(struct fs_device *device, off_t offset, void *buffer, size_t len, bool non_blocking) {
    (void) device;
    (void) offset;
    (void) buffer;
    (void) len;
    (void) non_blocking;

    return 0;
}

static ssize_t dev_zero_read(struct fs_device *device, off_t offset, void *buffer, size_t len, bool non_blocking) {
    (void) device;
    (void) offset;
    (void) non_blocking;

    memset(buffer, 0, len);
    return len;
}

static ssize_t dev_ignore_write(struct fs_device *device, off_t offset, const void *buffer, size_t len, bool non_blocking) {
    (void) device;
    (void) offset;
    (void) buffer;
    (void) non_blocking;

    return len;
}

static ssize_t full_write(struct fs_device *device, off_t offset, const void *buf, size_t n, bool non_blocking) {
    (void) device;
    (void) offset;
    (void) buf;
    (void) n;
    (void) non_blocking;

    return -ENOSPC;
}

static ssize_t urandom_read(struct fs_device *device, off_t offset, void *buf, size_t n, bool non_blocking) {
    (void) device;
    (void) offset;
    (void) non_blocking;

    uint8_t *buffer = buf;
    size_t i;
    for (i = 0; i <= n - 4; i += 4) {
        uint32_t random_data = get_random_bytes();
        *((uint32_t *) (buffer + i)) = random_data;
    }

    if (i != n) {
        uint32_t random_data = get_random_bytes();
        for (; i < n; i++) {
            buffer[i] = random_data & 0xFFU;
            random_data >>= 8U;
        }
    }

    return n;
}

static struct fs_device_ops dev_null_ops = {
    .read = &dev_null_read,
    .write = &dev_ignore_write,
};

static struct fs_device dev_null = {
    .name = "null",
    .device_number = 0x00101,
    .mode = S_IFCHR | 0666,
    .ops = &dev_null_ops,
    .lock = MUTEX_INITIALIZER(dev_null.lock),
};

static struct fs_device_ops dev_zero_ops = {
    .read = &dev_zero_read,
    .write = &dev_ignore_write,
};

static struct fs_device dev_zero = {
    .name = "zero",
    .device_number = 0x00102,
    .mode = S_IFCHR | 0666,
    .ops = &dev_zero_ops,
    .lock = MUTEX_INITIALIZER(dev_zero.lock),
};

static struct fs_device_ops dev_full_ops = {
    .read = &dev_zero_read,
    .write = &full_write,
};

static struct fs_device dev_full = {
    .name = "full",
    .device_number = 0x00103,
    .mode = S_IFCHR | 0666,
    .ops = &dev_full_ops,
    .lock = MUTEX_INITIALIZER(dev_full.lock),
};

static struct fs_device_ops dev_urandom_ops = {
    .read = &urandom_read,
    .write = dev_ignore_write,
};

static struct fs_device dev_urandom = {
    .name = "urandom",
    .device_number = 0x00104,
    .mode = S_IFCHR | 0666,
    .ops = &dev_urandom_ops,
    .lock = MUTEX_INITIALIZER(dev_urandom.lock),
};

void init_virtual_devices() {
    dev_register(&dev_null);
    dev_register(&dev_zero);
    dev_register(&dev_full);
    dev_register(&dev_urandom);

    init_ptmx();
}
