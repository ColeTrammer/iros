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
#include <kernel/util/random.h>

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

static ssize_t urandom_read(struct device *device, off_t offset, void *buf, size_t n) {
    (void) device;
    (void) offset;

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

static struct device_ops dev_null_ops = { .read = &dev_null_read, .write = &dev_ignore_write };

static struct device dev_null = { .device_number = 0x00101, .type = S_IFCHR, .ops = &dev_null_ops, .lock = MUTEX_INITIALIZER };

static struct device_ops dev_zero_ops = { .read = &dev_zero_read, .write = &dev_ignore_write };

static struct device dev_zero = { .device_number = 0x00102, .type = S_IFCHR, .ops = &dev_zero_ops, .lock = MUTEX_INITIALIZER };

static struct device_ops dev_full_ops = { .read = &full_read, .write = &full_write };

static struct device dev_full = { .device_number = 0x00103, .type = S_IFCHR, .ops = &dev_full_ops, .lock = MUTEX_INITIALIZER };

static struct device_ops dev_urandom_ops = { .read = &urandom_read, .write = dev_ignore_write };

static struct device dev_urandom = { .device_number = 0x00104, .type = S_IFCHR, .ops = &dev_urandom_ops, .lock = MUTEX_INITIALIZER };

void init_virtual_devices() {
    dev_register(&dev_null);
    dev_register(&dev_zero);
    dev_register(&dev_full);
    dev_register(&dev_urandom);
}
