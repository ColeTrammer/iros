#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/dev.h>
#include <kernel/hal/hw_device.h>
#include <kernel/hal/output.h>

#define HW_DEVICE_DEBUG

static void default_hw_device_destructor(struct hw_device *device) {
    free(device);
}

struct hw_device *create_hw_device(const char *name, struct hw_device *parent, struct fs_device *fs_device) {
    struct hw_device *device = malloc(sizeof(*device));
    init_hw_device(device, name, parent, fs_device, default_hw_device_destructor);
    return device;
}

void init_hw_device(struct hw_device *device, const char *name, struct hw_device *parent, struct fs_device *fs_device,
                    void (*destructor)(struct hw_device *device)) {
    strncpy(device->name, name, sizeof(device->name) - 1);
    device->name[sizeof(device->name) - 1] = '\0';

    init_list(&device->children);
    device->parent = parent;
    init_spinlock(&device->tree_lock);

    device->ref_count = 1;

    device->fs_device = fs_device;
    device->destructor = destructor;

    if (fs_device) {
        dev_register(fs_device);
    }
}

static void kill_hw_device(struct hw_device *device) {
#ifdef HW_DEVICE_DEBUG
    debug_log("Destroying hw device: [ %s ]\n", device->name);
#endif /* HW_DEVICE_DEBUG */

    struct hw_device *parent = device->parent;
    if (parent) {
        spin_lock(&parent->tree_lock);
        list_remove(&device->siblings);
        spin_unlock(&parent->tree_lock);
    }

    struct list_node *children = &device->children;
    list_for_each_entry(children, child, struct hw_device, siblings) {
        // Setting the parent to NULL will cause race conditions. This needs to be revisited.
        child->parent = NULL;
        drop_hw_device(child);
        list_remove(&child->siblings);
    }

    struct fs_device *fs_device = device->fs_device;
    if (fs_device) {
        dev_unregister(fs_device);
    }

    void (*destructor)(struct hw_device * self) = device->destructor;
    if (destructor) {
        destructor(device);
    }
}

void drop_hw_device(struct hw_device *device) {
    if (atomic_fetch_sub(&device->ref_count, 1) == 1) {
        kill_hw_device(device);
    }
}

struct hw_device *bump_hw_device(struct hw_device *device) {
    atomic_fetch_add(&device->ref_count, 1);
    return device;
}
