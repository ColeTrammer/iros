#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/mem/anon_vm_object.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/phys_page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>

// #define ANON_VM_OBJECT_DEBUG

static int anon_map(struct vm_object *self, struct vm_region *region) {
    struct anon_vm_object_data *data = self->private_data;

    struct task *current_task = get_current_task();

    spin_lock(&self->lock);
    for (uintptr_t i = region->start; i < region->end; i += PAGE_SIZE) {
        size_t page_index = (i + region->vm_object_offset - region->start) / PAGE_SIZE;
        assert(page_index < data->pages);

        struct phys_page *page = data->phys_pages[page_index];
        if (page) {
            if (atomic_load(&page->ref_count) == 1) {
                map_phys_page(page->phys_addr, i, region->flags, current_task->process);
            } else {
                map_phys_page(page->phys_addr, i, (region->flags & ~VM_WRITE) | VM_COW, current_task->process);
            }
        }
    }

    spin_unlock(&self->lock);
    return 0;
}

static int anon_kill(struct vm_object *self) {
    struct anon_vm_object_data *data = self->private_data;

    for (size_t i = 0; i < data->pages; i++) {
        if (data->phys_pages[i]) {
            drop_phys_page(data->phys_pages[i]);
        }
    }

    free(data);
    return 0;
}

static uintptr_t anon_handle_fault(struct vm_object *self, uintptr_t offset_into_self) {
    struct anon_vm_object_data *data = self->private_data;

    size_t page_index = offset_into_self / PAGE_SIZE;
    if (page_index >= data->pages) {
        debug_log("fault in address outside anon object: [ %#.16lX, %lu, %lu ]\n", offset_into_self, page_index, data->pages);
        assert(page_index < data->pages);
    }

    spin_lock(&self->lock);
    if (data->phys_pages[page_index]) {
        uintptr_t ret = data->phys_pages[page_index]->phys_addr;
        spin_unlock(&self->lock);
        return ret;
    }

    data->phys_pages[page_index] = allocate_phys_page();
    uintptr_t phys_addr = data->phys_pages[page_index]->phys_addr;

    void *phys_addr_mapping = create_phys_addr_mapping(phys_addr);
    memset(phys_addr_mapping, 0, PAGE_SIZE);

    spin_unlock(&self->lock);
    return phys_addr;
}

static uintptr_t anon_handle_cow_fault(struct vm_object *self, uintptr_t offset_into_self) {
    struct anon_vm_object_data *data = self->private_data;

    size_t page_index = offset_into_self / PAGE_SIZE;
    assert(page_index < data->pages);

    spin_lock(&self->lock);
    struct phys_page *old_page = data->phys_pages[page_index];
    assert(old_page);

    if (atomic_load(&old_page->ref_count) == 1) {
        spin_unlock(&self->lock);
        return old_page->phys_addr;
    }

    uintptr_t old_phys_addr = old_page->phys_addr;

    struct phys_page *new_page = allocate_phys_page();
    uintptr_t new_phys_addr = new_page->phys_addr;

    void *old_addr_mapping = create_phys_addr_mapping(old_phys_addr);
    void *new_addr_mapping = create_phys_addr_mapping(new_phys_addr);

    memcpy(new_addr_mapping, old_addr_mapping, PAGE_SIZE);
    data->phys_pages[page_index] = new_page;

    drop_phys_page(old_page);
    spin_unlock(&self->lock);

    return new_phys_addr;
}

static int anon_extend(struct vm_object *self, size_t pages) {
    struct anon_vm_object_data *data = self->private_data;

    size_t old_num_pages = data->pages;
    size_t num_pages = old_num_pages + pages;
#ifdef ANON_VM_OBJECT_DEBUG
    debug_log("anon_extend: [ %lu, %lu ]\n", pages, num_pages);
#endif /* ANON_VM_OBJECT_DEBUG */
    struct anon_vm_object_data *new_data = self->private_data =
        realloc(self->private_data, sizeof(struct anon_vm_object_data) + num_pages * sizeof(struct phys_page *));
    assert(new_data);

    new_data->pages = num_pages;
    memset(new_data->phys_pages + old_num_pages, 0, pages * sizeof(struct phys_page *));
    return 0;
}

static struct vm_object *anon_clone(struct vm_object *self);

static struct vm_object_operations anon_ops = { .map = &anon_map,
                                                .handle_fault = &anon_handle_fault,
                                                .handle_cow_fault = &anon_handle_cow_fault,
                                                .kill = &anon_kill,
                                                .extend = &anon_extend,
                                                .clone = &anon_clone };

static struct vm_object *anon_clone(struct vm_object *self) {
    struct anon_vm_object_data *self_data = self->private_data;

    spin_lock(&self->lock);

    struct anon_vm_object_data *data = malloc(sizeof(struct anon_vm_object_data) + self_data->pages * sizeof(struct phys_page *));
    data->pages = self_data->pages;

    for (size_t i = 0; i < self_data->pages; i++) {
        struct phys_page *page_value = self_data->phys_pages[i];
        if (!page_value) {
            data->phys_pages[i] = page_value;
        } else {
            data->phys_pages[i] = bump_phys_page(page_value);
        }
    }

    spin_unlock(&self->lock);
    return vm_create_object(VM_ANON, &anon_ops, data);
}

struct vm_object *vm_create_anon_object(size_t size) {
    size_t num_pages = ((size + PAGE_SIZE - 1) / PAGE_SIZE);
    struct anon_vm_object_data *data = malloc(sizeof(struct anon_vm_object_data) + num_pages * sizeof(struct phys_page *));
    assert(!size || data);

    data->pages = num_pages;
    memset(data->phys_pages, 0, num_pages * sizeof(struct phys_page *));
    return vm_create_object(VM_ANON, &anon_ops, data);
}
