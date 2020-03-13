#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/mem/inode_vm_object.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>

static int inode_map(struct vm_object *self, struct vm_region *region) {
    struct inode_vm_object_data *data = self->private_data;

    if (!data->inode_buffer) {
        data->inode_buffer = aligned_alloc(PAGE_SIZE, ((data->inode->size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE);
        assert(data->inode_buffer);

        int ret = fs_read_all_inode_with_buffer(data->inode, data->inode_buffer);
        if (ret < 0) {
            return ret;
        }
    }

    for (uintptr_t i = region->start; i < region->end; i += PAGE_SIZE) {
        map_phys_page(get_phys_addr((uintptr_t) data->inode_buffer + (i - region->start) + region->vm_object_offset), i, region->flags);
    }

    return 0;
}

static int inode_kill(struct vm_object *self) {
    struct inode_vm_object_data *data = self->private_data;

    debug_log("Destroying inode_vm_object: [ %p, %lu, %llu ]\n", self, data->inode->device, data->inode->index);

    if (data->inode_buffer && data->owned) {
        free(data->inode_buffer);
    }

    if (data->shared) {
        spin_lock(&data->inode->lock);
        data->inode->vm_object = NULL;
        spin_unlock(&data->inode->lock);
    }

    drop_inode_reference(data->inode);

    free(data);
    return 0;
}

static struct vm_object_operations inode_ops = { &inode_map, &inode_kill };

struct vm_object *vm_create_inode_object(struct inode *inode, int map_flags) {
    struct inode_vm_object_data *data = malloc(sizeof(struct inode_vm_object_data));
    assert(data);

    assert(inode->i_op->read_all);

    data->inode = inode;
    data->shared = (map_flags & MAP_SHARED) ? true : false;
    data->owned = true;
    data->inode_buffer = NULL;

    return vm_create_object(VM_INODE, &inode_ops, data);
}

struct vm_object *vm_create_direct_inode_object(struct inode *inode, void *base_buffer) {
    struct inode_vm_object_data *data = malloc(sizeof(struct inode_vm_object_data));
    assert(data);

    data->inode = inode;
    data->shared = true;
    data->owned = false;
    data->inode_buffer = base_buffer;

    return vm_create_object(VM_INODE, &inode_ops, data);
}