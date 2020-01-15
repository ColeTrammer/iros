#ifndef _KERNEL_MEM_ANON_VM_OBJECT_H
#define _KERNEL_MEM_ANON_VM_OBJECT_H 1

#include <kernel/mem/vm_object.h>

struct anon_vm_object_data {
    size_t pages;
    uintptr_t phys_pages[];
};

struct vm_object *vm_create_anon_object(size_t size);

#endif /* _KERNEL_MEM_ANON_VM_OBJECT_H */