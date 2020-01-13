#include <assert.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/mem/vm_object.h>

// #define VM_OBJECT_REF_COUNT_DEBUG

void drop_vm_object(struct vm_object *obj) {
    spin_lock(&obj->lock);
#ifdef VM_OBJECT_REF_COUNT_DEBUG
    debug_log("vm_object ref count: [ %p, %d ]\n", obj, obj->ref_count - 1);
#endif /* VM_OBJECT_REF_COUNT_DEBUG */
    assert(obj->ref_count > 0);
    if (--obj->ref_count == 0) {
        if (obj->ops->kill) {
            obj->ops->kill(obj);
        }

        spin_unlock(&obj->lock);
        free(obj);
        return;
    }

    spin_unlock(&obj->lock);
}

void bump_vm_object(struct vm_object *obj) {
    spin_lock(&obj->lock);
#ifdef VM_OBJECT_REF_COUNT_DEBUG
    debug_log("vm_object ref count: [ %p, %d ]\n", obj, obj->ref_count + 1);
#endif /* VM_OBJECT_REF_COUNT_DEBUG */
    obj->ref_count++;
    spin_unlock(&obj->lock);
}

struct vm_object *vm_create_object(enum vm_object_type type, struct vm_object_operations *ops, void *private_data) {
    assert(ops);

    struct vm_object *object = malloc(sizeof(struct vm_object));
    assert(object);

    object->type = type;
    object->ops = ops;
    object->private_data = private_data;
    object->ref_count = 1;
    init_spinlock(&object->lock);

    return object;
}