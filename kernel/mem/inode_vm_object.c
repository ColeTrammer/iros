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

        int error = 0;

        assert(data->inode->ref_count > 0);

        struct file *file = data->inode->i_op->open(data->inode, O_RDONLY, &error);
        if (!file) {
            return error;
        }

        ssize_t ret = fs_read(file, data->inode_buffer, data->inode->size);
        if (ret != (ssize_t) data->inode->size) {
            file->f_op->close(file);
            free(file);
            debug_log("Failed to read inode: [ %lu, %llu ]\n", file->device, file->inode_idenifier);
            return ret;
        }

        int rc = 0;
        if (file->f_op->close) {
            rc = file->f_op->close(file);
        }

        free(file);

        if (rc < 0) {
            return rc;
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

    if (data->inode_buffer) {
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

    data->inode = inode;
    data->shared = (map_flags & MAP_SHARED) ? true : false;
    data->inode_buffer = NULL;

    return vm_create_object(VM_INODE, &inode_ops, data);
}
