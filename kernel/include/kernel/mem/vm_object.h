#ifndef _KERNEL_MEM_VM_OBJECT_H
#define _KERNEL_MEM_VM_OBJECT_H 1

#include <stddef.h>

#include <kernel/util/spinlock.h>

struct vm_region;
struct vm_object;

enum vm_object_type { VM_INODE, VM_ANON, VM_PHYS };

struct vm_object_operations {
    int (*map)(struct vm_object *self, struct vm_region *region);
    int (*kill)(struct vm_object *self);
};

struct vm_object {
    enum vm_object_type type;
    int ref_count;

    spinlock_t lock;

    struct vm_object_operations *ops;
    void *private_data;
};

void drop_vm_object(struct vm_object *obj);
void bump_vm_object(struct vm_object *obj);
struct vm_object *vm_create_object(enum vm_object_type type, struct vm_object_operations *ops, void *private_data);

#endif /* _KERNEL_MEM_VM_OBJECT_H */