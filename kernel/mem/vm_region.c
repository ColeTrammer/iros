#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/hal/output.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/process.h>
#include <kernel/proc/profile.h>

// #define VM_REGION_DEBUG

struct vm_region *add_vm_region(struct vm_region *list, struct vm_region *to_add) {
    struct vm_region **link = &list;
    while (*link != NULL && (*link)->start < to_add->end) {
        link = &(*link)->next;
    }
    to_add->next = *link;
    *link = to_add;

    return list;
}

struct vm_region *remove_vm_region(struct vm_region *list, uint64_t type) {
    struct vm_region **link = &list;
    while (*link != NULL && (*link)->type != type) {
        link = &(*link)->next;
    }
    *link = (*link)->next;

#ifdef VM_REGION_DEBUG
    debug_log("VM Region Removed: [ %#.16lX, %#.16lX ]\n", type, (uintptr_t) list);
#endif /* VM_REGION_DEBUG */
    return list;
}

struct vm_region *get_vm_region(struct vm_region *list, uint64_t type) {
    while (list != NULL && list->type != type) {
        list = list->next;
    }
    return list;
}

int extend_vm_region_end(struct vm_region *list, uint64_t type, size_t num_pages) {
    list = get_vm_region(list, type);
    if (list == NULL) {
        return -1;
    }

    uintptr_t new_end = list->end + num_pages * PAGE_SIZE;
    if (list->next != NULL && new_end > list->next->start) {
        return -2; // Indicate there is no room
    }
    list->end = new_end;

    return 0;
}

int extend_vm_region_start(struct vm_region *list, uint64_t type, size_t num_pages) {
    struct vm_region *entry = list;
    if (entry == NULL) {
        return -1;
    }
    if (entry->type == type) {
        entry->start -= num_pages * PAGE_SIZE;
        return 0;
    }

    while (entry->next != NULL && entry->next->type != type) {
        entry = entry->next;
    }

    if (entry->next == NULL) {
        return -1;
    }

    uintptr_t new_start = entry->next->start - num_pages * PAGE_SIZE;
    if (entry->end > new_start) {
        return -2; // Indicate there is no room
    }
    entry->next->start = new_start;

#ifdef VM_REGION_DEBUG
    debug_log("VM Region Start Extended: [ %ld, %#.16lX, %#.16lX ]\n", num_pages, type, (uintptr_t) list);
#endif /* VM_REGION_DEBUG */
    return 0;
}

int contract_vm_region_end(struct vm_region *list, uint64_t type, size_t num_pages) {
    list = get_vm_region(list, type);
    if (list == NULL) {
        return -1;
    }

    uintptr_t new_end = list->end - num_pages * PAGE_SIZE;
    if (new_end < list->start) {
        return -2; // Indicate took away too much
    }
    list->end = new_end;

#ifdef VM_REGION_DEBUG
    debug_log("VM Region End Contracted: [ %ld, %#.16lX, %#.16lX ]\n", num_pages, type, (uintptr_t) list);
#endif /* VM_REGION_DEBUG */
    return 0;
}

int contract_vm_region_start(struct vm_region *list, uint64_t type, size_t num_pages) {
    list = get_vm_region(list, type);
    if (list == NULL) {
        return -1;
    }

    uintptr_t new_start = list->start + num_pages * PAGE_SIZE;
    if (new_start > list->end) {
        return -2; // Indicate took away too much
    }
    list->start = new_start;

#ifdef VM_REGION_DEBUG
    debug_log("VM Region Start Contracted: [ %ld, %#.16lX, %#.16lX ]\n", num_pages, type, (uintptr_t) list);
#endif /* VM_REGION_DEBUG */
    return 0;
}

struct vm_region *get_vm_last_region(struct vm_region *list, uint64_t type) {
    struct vm_region *region = NULL;
    while (list != NULL) {
        if (list->type == type) {
            region = list;
        }
        list = list->next;
    }
    return region;
}

int vm_map_region_with_object(struct vm_region *self) {
    assert(self->vm_object);
    assert(self->vm_object->ops);
    assert(self->vm_object->ops->map);
    struct process *process = get_current_process();
    if (!(self->flags & VM_NO_EXEC) && self->vm_object->type == VM_INODE) {
        if (atomic_load(&process->should_profile)) {
            // The profiler records the executable memory mapped regions so that the symbol information
            // of said reasons can be reconstructed in userspace.
            proc_record_memory_map(process);
        }
    }
    return self->vm_object->ops->map(self->vm_object, self);
}

const char *vm_type_to_string(uint64_t type) {
    switch (type) {
        case VM_KERNEL_PHYS_ID:
            return "kernel=>physical id map";
        case VM_KERNEL_TEXT:
            return "kernel=>text";
        case VM_KERNEL_ROD:
            return "kernel=>read only data";
        case VM_KERNEL_DATA:
            return "kernel=>data";
        case VM_INITRD:
            return "kernel=>initial ram disk";
        case VM_KERNEL_HEAP:
            return "kernel=>heap";
        case VM_KERNEL_ANON_MAPPING:
            return "kernel=>anonymous mapping";
        case VM_KERNEL_DMA_MAPPING:
            return "kernel=>dma mapping";
        case VM_KERNEL_STACK:
            return "kernel=>stack";
        case VM_PROCESS_TEXT:
            return "text";
        case VM_PROCESS_ROD:
            return "read only data";
        case VM_PROCESS_DATA:
            return "data";
        case VM_PROCESS_BSS:
            return "bss";
        case VM_PROCESS_HEAP:
            return "heap";
        case VM_PROCESS_TLS_MASTER_COPY:
            return "thread local storage";
        case VM_TASK_STACK:
            return "stack";
        case VM_TASK_STACK_GUARD:
            return "stack guard";
        case VM_PROCESS_FILE:
            return "file";
        case VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES:
            return "device map";
        case VM_PROCESS_ANON_MAPPING:
            return "anonymous mapping";
        default:
            return "unknown";
    }
}
