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

static struct device *slaves[PTMX_MAX] = { 0 };
static struct device *masters[PTMX_MAX] = { 0 };
static spinlock_t lock = SPINLOCK_INITIALIZER;

static void slave_on_open(struct device *device) {
    struct slave_data *data = device->private;
    assert(data);

    spin_lock(&data->lock);
    data->ref_count++;
    spin_unlock(&data->lock);
}

static int slave_close(struct device *device) {
    struct slave_data *data = device->private;
    assert(data);

    spin_lock(&data->lock);
    data->ref_count--;
    if (data->ref_count <= 0) {
        // data->lock will be unlocked in remove callback
        dev_remove(device->name);
        return 0;
    }

    spin_unlock(&data->lock);
    return 0;
}

static void slave_add(struct device *device) {
    struct slave_data *data = calloc(1, sizeof(struct slave_data));
    init_spinlock(&data->lock);
    data->ref_count = 1; // For the master

    for (int i = 0; i < PTMX_MAX; i++) {
        if (device == slaves[i]) {
            data->index = i;
            break;
        }
    }

    device->private = data;
}

static void slave_remove(struct device *device) {
    struct slave_data *data = device->private;
    assert(data);

    spin_unlock(&data->lock);

    debug_log("Removing slave tty: [ %d ]\n", data->index);

    slaves[data->index] = NULL;
    free(data);
}

static struct device_ops slave_ops = {
    NULL, NULL, NULL, slave_close, slave_add, slave_remove, NULL, slave_on_open, NULL
};

static void master_on_open(struct device *device) {
    device->cannot_open = true;
}

static int master_close(struct device *device) {
    dev_remove(device->name);
    return 0;
}

static void master_add(struct device *device) {
    struct master_data *data = calloc(1, sizeof(struct master_data));
    for (int i = 0; i < PTMX_MAX; i++) {
        if (device == masters[i]) {
            data->index = i;
            break;
        }
    }

    device->private = data;
}

static void master_remove(struct device *device) {
    struct master_data *data = device->private;
    assert(data);

    debug_log("Removing master tty: [ %d ]\n", data->index);

    slave_close(slaves[data->index]);

    masters[data->index] = NULL;
    free(data);
}

static struct device_ops master_ops = {
    NULL, NULL, NULL, master_close, master_add, master_remove, NULL, master_on_open, NULL
};

static struct file *ptmx_open(struct device *device, int flags, int *error) {
    (void) device;

    debug_log("Opening ptmx\n");

    spin_lock(&lock);
    for (int i = 0; i < PTMX_MAX; i++) {
        if (slaves[i] == NULL && masters[i] == NULL) {
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
    ptmx_open, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
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