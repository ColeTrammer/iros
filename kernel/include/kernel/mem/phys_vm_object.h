#ifndef _KERNEL_MEM_PHYS_VM_OBJECT_H
#define _KERNEL_MEM_PHYS_VM_OBJECT_H 1

#include <kernel/mem/vm_object.h>

struct phys_vm_object_data {
    uintptr_t phys_start;
    size_t size;
    int (*on_kill)(void *closure);
    void *closure;
};

int inode_on_kill(void *inode);

struct vm_object *vm_create_phys_object(uintptr_t phys_end, size_t size, int (*on_kill)(void *closure), void *closure);

#endif /* _KERNEL_MEM_PHYS_VM_OBJECT_H */