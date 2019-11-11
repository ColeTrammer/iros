#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/file.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/ptmx.h>
#include <kernel/hal/tty.h>
#include <kernel/util/spinlock.h>

#define PTMX_MAX 16

static struct device_ops slave_ops = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static struct device_ops master_ops = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static struct device *slaves[16] = { 0 };
static struct device *masters[16] = { 0 };
static spinlock_t lock = SPINLOCK_INITIALIZER;

static struct file *ptmx_open(struct device *device, int flags, int *error) {
    (void) device;

    debug_log("Opening ptmx\n");

    spin_lock(&lock);
    for (int i = 0; i < PTMX_MAX; i++) {
        if (slaves[i] == NULL) {
            slaves[i] = calloc(1, sizeof(struct device));
            spin_unlock(&lock);

            slaves[i]->device_number = 0x5000 + i;
            snprintf(slaves[i]->name, sizeof(slaves[i]->name - 1), "tty%d", i);
            slaves[i]->ops = &slave_ops;
            slaves[i]->type = S_IFCHR;
            slaves[i]->private = NULL;

            struct device *master = calloc(1, sizeof(struct device));
            master->device_number = 0x10000 + i;
            snprintf(master->name, 7, "mtty%d",i);
            master->ops = &master_ops;
            master->type = S_IFCHR;
            master->private = NULL;
            masters[i] = master;

            dev_add(masters[i], masters[i]->name);

            dev_add(slaves[i], slaves[i]->name);

            char path[16] = { 0 };
            snprintf(path, 15, "/dev/mtty%d", i);
            debug_log("Opening: [ %s ]\n", path);
            return fs_open(path, flags, error);
        }
    }

    spin_unlock(&lock);
    *error = -ENOMEM;
    return NULL;
}

struct device_ops ptmx_ops = {
    ptmx_open, NULL, NULL, NULL, NULL, NULL, NULL
};

void init_ptmx() {
    struct device *ptmx_device = calloc(1, sizeof(struct device));
    ptmx_device->device_number = 0x7500;
    strcpy(ptmx_device->name, "ptmx");
    ptmx_device->ops = &ptmx_ops;
    ptmx_device->private = NULL;
    ptmx_device->type = S_IFCHR;

    dev_add(ptmx_device, ptmx_device->name);
}