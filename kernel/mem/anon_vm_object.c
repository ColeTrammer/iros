#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/mem/anon_vm_object.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>

static int anon_map(struct vm_object *self, struct vm_region *region) {
    struct anon_vm_object_data *data = self->private_data;

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

static int anon_kill(struct vm_object *self) {
    struct anon_vm_object_data *data = self->private_data;

    struct process *process = get_current_task()->process;
    for (size_t i = 0; i < data->pages; i++) {
        if (data->phys_pages[i]) {
            free_phys_page(data->phys_pages[i], process);
        }
    }

    free(data);
    return 0;
}

static uintptr_t anon_handle_fault(struct vm_object *self, uintptr_t offset_into_self) {
    struct anon_vm_object_data *data = self->private_data;

    struct task *current_task = get_current_task();

    size_t page_index = offset_into_self / PAGE_SIZE;
    assert(page_index < data->pages);

    spin_lock(&self->lock);

    assert(!data->phys_pages[page_index]);
    size_t phys_addr = get_next_phys_page(current_task->process);
    data->phys_pages[page_index] = phys_addr;

    void *phys_addr_mapping = create_phys_addr_mapping(phys_addr);
    memset(phys_addr_mapping, 0, PAGE_SIZE);

    spin_unlock(&self->lock);
    return data->phys_pages[page_index];
}

static int anon_extend(struct vm_object *self, size_t pages) {
    struct anon_vm_object_data *data = self->private_data;

    size_t old_num_pages = data->pages;
    size_t num_pages = old_num_pages + pages;
    struct anon_vm_object_data *new_data = self->private_data =
        realloc(self->private_data, sizeof(struct anon_vm_object_data) + num_pages * sizeof(uintptr_t));
    assert(new_data);

    new_data->pages = num_pages;
    memset(new_data->phys_pages + old_num_pages, 0, (num_pages - old_num_pages) * sizeof(uintptr_t));
    return 0;
}

static struct vm_object *anon_clone(struct vm_object *self);

static struct vm_object_operations anon_ops = {
    .map = &anon_map, .handle_fault = &anon_handle_fault, .kill = &anon_kill, .extend = &anon_extend, .clone = &anon_clone
};

static struct vm_object *anon_clone(struct vm_object *self) {

    struct anon_vm_object_data *self_data = self->private_data;

    struct process *process = get_current_task()->process;

    spin_lock(&temp_page_lock);
    spin_lock(&self->lock);

    struct anon_vm_object_data *data = malloc(sizeof(struct anon_vm_object_data) + self_data->pages * sizeof(uintptr_t));
    data->pages = self_data->pages;

    for (size_t i = 0; i < self_data->pages; i++) {
        uintptr_t page_value = self_data->phys_pages[i];
        if (!page_value) {
            data->phys_pages[i] = page_value;
        } else {
            map_page((uintptr_t) TEMP_PAGE, VM_WRITE, process);
            data->phys_pages[i] = get_phys_addr((uintptr_t) TEMP_PAGE);

            void *source = create_phys_addr_mapping(self_data->phys_pages[i]);
            memcpy(TEMP_PAGE, source, PAGE_SIZE);
        }
    }

    spin_unlock(&self->lock);
    spin_unlock(&temp_page_lock);

    return vm_create_object(VM_ANON, &anon_ops, data);
}

struct vm_object *vm_create_anon_object(size_t size) {
    size_t num_pages = ((size + PAGE_SIZE - 1) / PAGE_SIZE);
    struct anon_vm_object_data *data = malloc(sizeof(struct anon_vm_object_data) + num_pages * sizeof(uintptr_t));
    assert(!size || data);

    data->pages = num_pages;
    memset(data->phys_pages, 0, num_pages * sizeof(uintptr_t));
    return vm_create_object(VM_ANON, &anon_ops, data);
}