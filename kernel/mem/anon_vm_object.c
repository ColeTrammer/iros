#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/mem/anon_vm_object.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>

static int anon_map(struct vm_object *self, struct vm_region *region) {
    struct anon_vm_object_data *data = self->private_data;

    for (uintptr_t i = region->start; i < region->end; i += PAGE_SIZE) {
        size_t page_index = (i + region->vm_object_offset - region->start) / PAGE_SIZE;
        assert(page_index < data->pages);

        if (!data->phys_pages[page_index]) {
            data->phys_pages[page_index] = get_next_phys_page();
        }

        map_phys_page(data->phys_pages[page_index], i, region->flags);
    }

    return 0;
}

static int anon_kill(struct vm_object *self) {
    struct anon_vm_object_data *data = self->private_data;

    for (size_t i = 0; i < data->pages; i++) {
        if (data->phys_pages[i]) {
            free_phys_page(data->phys_pages[i]);
        }
    }

    free(data);
    return 0;
}

static struct vm_object_operations anon_ops = { &anon_map, &anon_kill };

struct vm_object *vm_create_anon_object(size_t size) {
    struct anon_vm_object_data *data = malloc(sizeof(struct anon_vm_object_data) + size / PAGE_SIZE * sizeof(uintptr_t));
    assert(data);

    data->pages = ((size + PAGE_SIZE - 1) / PAGE_SIZE);
    memset(data->phys_pages, 0, data->pages * sizeof(uintptr_t));
    return vm_create_object(VM_ANON, &anon_ops, data);
}