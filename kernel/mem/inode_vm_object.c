#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/mem/inode_vm_object.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>

static int inode_map(struct vm_object *self, struct vm_region *region) {
    struct inode_vm_object_data *data = self->private_data;

    struct task *current_task = get_current_task();

    spin_lock(&self->lock);
    for (uintptr_t i = region->start; i < region->end; i += PAGE_SIZE) {
        size_t page_index = (i + region->vm_object_offset - region->start) / PAGE_SIZE;
        assert(page_index < data->pages);

        if (data->phys_pages[page_index]) {
            map_phys_page(data->phys_pages[page_index], i, region->flags, current_task->process);
        }
    }

    spin_unlock(&self->lock);
    return 0;
}

static uintptr_t inode_handle_fault(struct vm_object *self, uintptr_t offset_into_self) {
    struct inode_vm_object_data *data = self->private_data;

    size_t page_index = offset_into_self / PAGE_SIZE;
    assert(page_index < data->pages);

    spin_lock(&self->lock);
    assert(!data->phys_pages[page_index]);
    uintptr_t phys_addr = get_next_phys_page(get_current_task()->process);
    data->phys_pages[page_index] = phys_addr;
    char *phys_page_mapping = create_phys_addr_mapping(phys_addr);

    struct inode *inode = data->inode;
    ssize_t read = inode->i_op->read(inode, phys_page_mapping, PAGE_SIZE, page_index * PAGE_SIZE);
    if (read == -1) {
        read = 0;
    }

    memset(phys_page_mapping + read, 0, PAGE_SIZE - read);
    spin_unlock(&self->lock);
    return phys_addr;
}

static int inode_kill(struct vm_object *self) {
    struct inode_vm_object_data *data = self->private_data;

    debug_log("Destroying inode_vm_object: [ %p, %lu, %llu ]\n", self, data->inode->device, data->inode->index);

    if (data->owned) {
        struct process *process = get_current_task()->process;
        for (size_t i = 0; i < data->pages; i++) {
            if (data->phys_pages[i]) {
                free_phys_page(data->phys_pages[i], process);
            }
        }
    }

    spin_lock(&data->inode->lock);
    data->inode->vm_object = NULL;
    drop_inode_reference_unlocked(data->inode);
    spin_unlock(&data->inode->lock);

    free(data);
    return 0;
}

static struct vm_object_operations inode_ops = { .map = &inode_map, .handle_fault = &inode_handle_fault, .kill = &inode_kill };

struct vm_object *vm_create_inode_object(struct inode *inode, int map_flags __attribute__((unused))) {
    size_t num_pages = ((inode->size + PAGE_SIZE - 1) / PAGE_SIZE);
    struct inode_vm_object_data *data = malloc(sizeof(struct inode_vm_object_data) + num_pages * sizeof(uintptr_t));
    assert(data);

    assert(inode->i_op->read);

    data->inode = inode;
    data->owned = true;
    data->pages = num_pages;
    memset(data->phys_pages, 0, num_pages * sizeof(uintptr_t));

    return vm_create_object(VM_INODE, &inode_ops, data);
}

struct vm_object *vm_create_direct_inode_object(struct inode *inode, void *base_buffer) {
    size_t num_pages = ((inode->size + PAGE_SIZE - 1) / PAGE_SIZE);
    struct inode_vm_object_data *data = malloc(sizeof(struct inode_vm_object_data) + num_pages * sizeof(uintptr_t));
    assert(data);

    data->inode = inode;
    data->owned = false;
    data->pages = num_pages;

    char *buffer = base_buffer;
    for (size_t i = 0; i < num_pages; i++) {
        data->phys_pages[i] = get_phys_addr((uintptr_t)(buffer + (i * PAGE_SIZE)));
    }

    // Make sure to zero excess bytes before allowing the pages to be mapped in
    memset(buffer + inode->size, 0, num_pages * PAGE_SIZE - inode->size);
    return vm_create_object(VM_INODE, &inode_ops, data);
}