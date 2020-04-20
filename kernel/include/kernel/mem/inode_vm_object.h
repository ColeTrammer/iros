#ifndef _KERNEL_MEM_INODE_VM_OBJECT_H
#define _KERNEL_MEM_INODE_VM_OBJECT_H 1

#include <stdbool.h>

#include <kernel/mem/vm_object.h>

struct inode_vm_object_data {
    struct inode *inode;
    bool owned;
    size_t pages;
    struct phys_page *phys_pages[];
};

struct vm_object *vm_create_inode_object(struct inode *inode, int map_flags);
struct vm_object *vm_create_direct_inode_object(struct inode *inode, void *base_buffer);

#endif /* _KERNEL_MEM_INODE_VM_OBJECT_H */