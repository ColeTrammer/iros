#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <sys/types.h>

#include <kernel/fs/dev.h>

static ssize_t dev_null_read(struct device *device, struct file *file, void *buffer, size_t len) {
    (void) device;
    (void) file;
    (void) buffer;
    (void) len;    

    return 0;
}

static ssize_t dev_zero_read(struct device *device, struct file *file, void *buffer, size_t len) {
    (void) device;
    (void) file;

    memset(buffer, 0, len);
    return len;
}

static ssize_t dev_ignore_write(struct device *device, struct file *file, const void *buffer, size_t len) {
    (void) device;
    (void) file;
    (void) buffer;

    return len;
}

static struct device_ops dev_null_ops = {
    NULL, &dev_null_read, &dev_ignore_write, NULL, NULL, NULL, NULL
};

static struct device dev_null = {
    0x31, S_IFCHR, "null", &dev_null_ops, NULL
};

static struct device_ops dev_zero_ops = {
    NULL, &dev_zero_read, &dev_ignore_write, NULL, NULL, NULL, NULL
};

static struct device dev_zero = {
    0x32, S_IFCHR, "zero", &dev_zero_ops, NULL
};

void init_virtual_devices() {
    dev_add(&dev_null, "null");
    dev_add(&dev_zero, "zero");
}