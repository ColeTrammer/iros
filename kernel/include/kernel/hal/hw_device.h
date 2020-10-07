#ifndef _KERNEL_HAL_HW_DEVICE_H
#define _KERNEL_HAL_HW_DEVICE_H 1

#include <kernel/util/list.h>
#include <kernel/util/spinlock.h>

struct fs_device;

struct hw_device {
    char name[16];

    struct list_node siblings;
    struct list_node children;
    struct hw_device *parent;
    spinlock_t tree_lock;

    int ref_count;

    struct fs_device *fs_device;
    void (*destructor)(struct hw_device *self);
};

struct hw_device *create_hw_device(const char *name, struct hw_device *parent, struct fs_device *fs_device);
void init_hw_device(struct hw_device *device, const char *name, struct hw_device *parent, struct fs_device *fs_device,
                    void (*destructor)(struct hw_device *device));
void drop_hw_device(struct hw_device *device);
struct hw_device *bump_hw_device(struct hw_device *device);
struct hw_device *root_hw_device(void);

#endif /* _KERNEL_HAL_HW_DEVICE_H */
