#ifndef _KERNEL_HAL_HW_DEVICE_H
#define _KERNEL_HAL_HW_DEVICE_H 1

#include <stdint.h>

#include <kernel/util/list.h>
#include <kernel/util/spinlock.h>

struct fs_device;
struct hw_device;

enum hw_device_type {
    HW_TYPE_NONE,
    HW_TYPE_ISA,
    HW_TYPE_PS2,
    HW_TYPE_PCI,
};

enum hw_device_status {
    HW_STATUS_DETECTED,
    HW_STATUS_ACTIVE,
    HW_STATUS_REMOVED,
};

struct ps2_device_id {
    uint8_t byte0;
    uint8_t byte1;
};

struct pci_device_id {
    uint16_t vendor_id;
    uint16_t device_id;
};

struct hw_device_id {
    enum hw_device_type type;
    union {
        struct ps2_device_id ps2_id;
        struct pci_device_id pci_id;
    };
};

struct hw_device {
    char name[64];

    struct list_node siblings;
    struct list_node children;
    struct hw_device *parent;
    spinlock_t tree_lock;

    int ref_count;
    enum hw_device_status status;
    struct hw_device_id id;

    struct fs_device *fs_device;
    void (*destructor)(struct hw_device *self);
};

const char *hw_type_to_string(enum hw_device_type type);
const char *hw_status_to_string(enum hw_device_status status);

struct hw_device *create_hw_device(const char *name, struct hw_device *parent, struct hw_device_id id, struct fs_device *fs_device);
void init_hw_device(struct hw_device *device, const char *name, struct hw_device *parent, struct hw_device_id id,
                    struct fs_device *fs_device, void (*destructor)(struct hw_device *device));
void drop_hw_device(struct hw_device *device);
int show_hw_device_id(struct hw_device_id id, char *buffer, size_t buffer_length);
int show_hw_device(struct hw_device *device, char *buffer, size_t buffer_length);
void remove_hw_device(struct hw_device *device);
struct hw_device *bump_hw_device(struct hw_device *device);
struct hw_device *root_hw_device(void);

static inline struct hw_device_id hw_device_id_none(void) {
    return (struct hw_device_id) { .type = HW_TYPE_NONE };
}

static inline struct hw_device_id hw_device_id_isa(void) {
    return (struct hw_device_id) { .type = HW_TYPE_ISA };
}

static inline struct hw_device_id hw_device_id_ps2(struct ps2_device_id id) {
    return (struct hw_device_id) { .type = HW_TYPE_PS2, .ps2_id = id };
}

static inline struct hw_device_id hw_device_id_pci(struct pci_device_id id) {
    return (struct hw_device_id) { .type = HW_TYPE_PCI, .pci_id = id };
}

#endif /* _KERNEL_HAL_HW_DEVICE_H */
