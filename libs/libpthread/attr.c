#define __libc_internal

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>

int pthread_attr_destroy(pthread_attr_t *attr) {
    attr->__flags = -1;
    return 0;
}

int pthread_attr_init(pthread_attr_t *attr) {
    attr->__flags = PTHREAD_CREATE_JOINABLE | PTHREAD_INHERIT_SCHED;
    attr->__guard_size = PAGE_SIZE;
    attr->__stack_start = NULL;
    attr->__stack_len = 2 * 1024 * 1024;
    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *__restrict attr, int *__restrict detachstate) {
    if (attr == NULL || attr->__flags == -1) {
        return EINVAL;
    }

    *detachstate = attr->__flags & PTHREAD_CREATE_DETACHED;
    return 0;
}

int pthread_attr_getguardsize(const pthread_attr_t *__restrict attr, size_t *__restrict guardsize) {
    if (attr == NULL || attr->__flags == -1 || guardsize == NULL) {
        return EINVAL;
    }

    *guardsize = attr->__guard_size;
    return 0;
}

int pthread_attr_getinheritsched(const pthread_attr_t *__restrict attr, int *__restrict inheritsched) {
    if (attr == NULL || attr->__flags == -1 || inheritsched == NULL) {
        return EINVAL;
    }

    *inheritsched = attr->__flags & PTHREAD_EXPLICIT_SCHED;
    return 0;
}

int pthread_attr_getscope(pthread_attr_t *__restrict attr, int *__restrict scope) {
    if (attr == NULL || attr->__flags == -1 || scope == NULL) {
        return EINVAL;
    }

    *scope = PTHREAD_SCOPE_SYSTEM;
    return 0;
}

int pthread_attr_getstack(const pthread_attr_t *__restrict attr, void **__restrict stackaddr, size_t *__restrict stacksize) {
    if (attr == NULL || attr->__flags == -1 || stackaddr == NULL || stacksize == NULL) {
        return EINVAL;
    }

    *stackaddr = attr->__stack_start;
    *stacksize = attr->__stack_len;
    return 0;
}

int pthread_attr_getstackaddr(const pthread_attr_t *__restrict attr, void **__restrict stackaddr) {
    if (attr == NULL || attr->__flags == -1 || stackaddr == NULL) {
        return EINVAL;
    }

    *stackaddr = attr->__stack_start;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *__restrict attr, size_t *__restrict stacksize) {
    if (attr == NULL || attr->__flags == -1 || stacksize == NULL) {
        return EINVAL;
    }

    *stacksize = attr->__stack_len;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    if (detachstate != PTHREAD_CREATE_DETACHED && detachstate != PTHREAD_CREATE_JOINABLE) {
        return EINVAL;
    }

    attr->__flags |= detachstate & PTHREAD_CREATE_DETACHED;
    return 0;
}

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize) {
    if (attr == NULL || attr->__flags == -1) {
        return EINVAL;
    }

    attr->__guard_size = guardsize;
    return 0;
}

int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched) {
    if (attr == NULL || attr->__flags == -1 || (inheritsched != PTHREAD_INHERIT_SCHED && inheritsched != PTHREAD_EXPLICIT_SCHED)) {
        return EINVAL;
    }

    attr->__flags |= inheritsched;
    return 0;
}

int pthread_attr_setscope(const pthread_attr_t *attr, int scope) {
    if (attr == NULL || attr->__flags == -1 || scope != PTHREAD_SCOPE_SYSTEM) {
        return EINVAL;
    }

    return 0;
}

int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize) {
    if (attr == NULL || attr->__flags == -1 || stacksize < PTHREAD_STACK_MIN) {
        return EINVAL;
    }

    attr->__stack_start = stackaddr;
    attr->__stack_len = stacksize;
    if (stackaddr != NULL) {
        attr->__flags |= __PTHREAD_MAUALLY_ALLOCATED_STACK;
    } else {
        attr->__flags &= ~__PTHREAD_MAUALLY_ALLOCATED_STACK;
    }

    return 0;
}

int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr) {
    if (attr == NULL || attr->__flags == -1) {
        return EINVAL;
    }

    attr->__stack_start = stackaddr;
    if (stackaddr != NULL) {
        attr->__flags |= __PTHREAD_MAUALLY_ALLOCATED_STACK;
    } else {
        attr->__flags &= ~__PTHREAD_MAUALLY_ALLOCATED_STACK;
    }

    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    if (attr == NULL || attr->__flags == -1 || stacksize < PTHREAD_STACK_MIN) {
        return EINVAL;
    }

    attr->__stack_len = stacksize;
    return 0;
}