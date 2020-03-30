#ifndef _KERNEL_MEM_VM_OBJECT_H
#define _KERNEL_MEM_VM_OBJECT_H 1

#include <stddef.h>

#include <kernel/util/spinlock.h>

struct vm_region;
struct vm_object;

enum vm_object_type { VM_INODE, VM_ANON, VM_PHYS };

struct vm_object_operations {
    int (*map)(struct vm_object *self, struct vm_region *region);
    uintptr_t (*handle_fault)(struct vm_object *self, uintptr_t offset_into_self);
    int (*kill)(struct vm_object *self);
    int (*extend)(struct vm_object *self, size_t pages);
    struct vm_object *(*clone)(struct vm_object *self);
};

struct vm_object {
    enum vm_object_type type;
    int ref_count;

    spinlock_t lock;

    struct vm_object_operations *ops;
    void *private_data;
};

void drop_vm_object(struct vm_object *obj);
struct vm_object *bump_vm_object(struct vm_object *obj);
struct vm_object *vm_create_object(enum vm_object_type type, struct vm_object_operations *ops, void *private_data);
struct vm_object *vm_clone_object(struct vm_object *obj);

int vm_handle_fault_in_region(struct vm_region *region, uintptr_t address);

#endif /* _KERNEL_MEM_VM_OBJECT_H */