#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>

#include <string.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_object.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>

// #define VM_OBJECT_REF_COUNT_DEBUG

void drop_vm_object(struct vm_object *obj) {
    int fetched_ref_count = atomic_fetch_sub(&obj->ref_count, 1);
#ifdef VM_OBJECT_REF_COUNT_DEBUG
    debug_log("vm_object ref count: [ %p, %d ]\n", obj, fetched_ref_count - 1);
#endif /* VM_OBJECT_REF_COUNT_DEBUG */
    assert(fetched_ref_count > 0);
    if (fetched_ref_count == 1) {
#ifdef VM_OBJECT_REF_COUNT_DEBUG
        debug_log("vm_object->kill: [ %p ]\n", obj);
#endif /* VM_OBJECT_REF_COUNT_DEBUG */

        if (obj->ops->kill) {
            obj->ops->kill(obj);
        }

        free(obj);
        return;
    }
}

struct vm_object *bump_vm_object(struct vm_object *obj) {
    int fetched_ref_count = atomic_fetch_add(&obj->ref_count, 1);
    (void) fetched_ref_count;
#ifdef VM_OBJECT_REF_COUNT_DEBUG
    debug_log("vm_object ref count: [ %p, %d ]\n", obj, fetched_ref_count + 1);
#endif /* VM_OBJECT_REF_COUNT_DEBUG */
    return obj;
}

int vm_handle_cow_fault_in_region(struct vm_region *region, uintptr_t address) {
    address &= ~(PAGE_SIZE - 1);

    struct process *process = get_current_task()->process;

    struct vm_object *object = region->vm_object;
    uintptr_t offset_in_object = region->vm_object_offset + address - region->start;

    if (!object->ops->handle_cow_fault) {
        debug_log("unrecoverable vm_object cow fault: [ %#.16lX ]\n", address);
        return 1;
    }

    uintptr_t phys_address_to_map = object->ops->handle_cow_fault(object, offset_in_object);
    map_phys_page(phys_address_to_map, address, region->flags, process);
    return 0;
}

int vm_handle_fault_in_region(struct vm_region *region, uintptr_t address) {
    address &= ~(PAGE_SIZE - 1);

    struct process *process = get_current_task()->process;
    if (!region->vm_object) {
        map_page(address, region->flags, process);
        memset((void *) address, 0, PAGE_SIZE);
        return 0;
    }

    struct vm_object *object = region->vm_object;
    uintptr_t offset_in_object = region->vm_object_offset + address - region->start;

    if (!object->ops->handle_fault) {
        debug_log("unrecoverable vm_object fault: [ %#.16lX ]\n", address);
        return 1;
    }

    uintptr_t phys_address_to_map = object->ops->handle_fault(object, offset_in_object);
    map_phys_page(phys_address_to_map, address, region->flags, process);
    return 0;
}

struct vm_object *vm_create_object(enum vm_object_type type, struct vm_object_operations *ops, void *private_data) {
    assert(ops);

    struct vm_object *object = malloc(sizeof(struct vm_object));
    assert(object);

    object->type = type;
    object->ops = ops;
    object->private_data = private_data;
    object->ref_count = 1;
    init_mutex(&object->lock);

    return object;
}

struct vm_object *vm_clone_object(struct vm_object *obj) {
    assert(obj->ops->clone);
    return obj->ops->clone(obj);
}
