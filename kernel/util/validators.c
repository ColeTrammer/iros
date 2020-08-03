#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>

#include <kernel/fs/vfs.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/util/validators.h>

int validate_string(const char *s, int unused) {
    (void) unused;

    struct vm_region *region = find_user_vm_region_by_addr((uintptr_t) s);
    if (!region || (region->flags & VM_PROT_NONE)) {
        return -EFAULT;
    }

    for (size_t i = 0; i < 16 * PAGE_SIZE; i++) {
        if ((uintptr_t)(s + i) > region->end) {
            region = find_user_vm_region_by_addr((uintptr_t)(s + i));
            if (!region || (region->flags & VM_PROT_NONE)) {
                return -EFAULT;
            }
        }

        if (s[i] == '\0') {
            return 0;
        }
    }

    return -E2BIG;
}

int validate_string_array(char **arr, int unused) {
    (void) unused;

    struct vm_region *region = find_user_vm_region_by_addr((uintptr_t) arr);
    if (!region || (region->flags & VM_PROT_NONE)) {
        return -EFAULT;
    }

    for (size_t i = 0; i < 8192; i++) {
        if ((uintptr_t)(arr + i) > region->end) {
            region = find_user_vm_region_by_addr((uintptr_t)(arr + i));
            if (!region || (region->flags & VM_PROT_NONE)) {
                return -EFAULT;
            }
        }

        char *arg = arr[i];
        if (!arg) {
            return 0;
        }

        VALIDATE(arg, -1, validate_string);
    }

    return -E2BIG;
}

int validate_path_or_null(const char *s, int dont_tolerate_null_after_all) {
    if (s == NULL && !dont_tolerate_null_after_all) {
        return 0;
    }

    return validate_path(s, 0);
}

int validate_path(const char *s, int unused) {
    (void) unused;

    struct vm_region *region = find_user_vm_region_by_addr((uintptr_t) s);
    if (!region || (region->flags & VM_PROT_NONE)) {
        return -EFAULT;
    }

    size_t component_length = 0;
    for (size_t i = 0; i <= PATH_MAX; i++) {
        if ((uintptr_t)(s + i) > region->end) {
            region = find_user_vm_region_by_addr((uintptr_t)(s + i));
            if (!region || (region->flags & VM_PROT_NONE)) {
                return -EFAULT;
            }
        }

        if (component_length > NAME_MAX) {
            return -ENAMETOOLONG;
        } else if (s[i] == '\0') {
            return 0;
        } else if (s[i] == '/') {
            component_length = 0;
        } else {
            component_length++;
        }
    }

    return -ENAMETOOLONG;
}

int validate_write(void *buffer, size_t size) {
    if (!buffer) {
        return -EFAULT;
    }

    uintptr_t offset = 0;
    struct vm_region *region;
    do {
        region = find_user_vm_region_by_addr((uintptr_t) buffer + offset);
        if (!region || !(region->flags & VM_WRITE)) {
            return -EFAULT;
        }
        offset = region->end - (uintptr_t) buffer;
    } while ((uintptr_t) buffer + size > region->end);

    return 0;
}

int validate_read(const void *buffer, size_t size) {
    if (!buffer) {
        return -EFAULT;
    }

    uintptr_t offset = 0;
    struct vm_region *region;
    do {
        region = find_user_vm_region_by_addr((uintptr_t) buffer + offset);
        if (!region || (region->flags & VM_PROT_NONE)) {
            return -EFAULT;
        }
        offset = region->end - (uintptr_t) buffer;
    } while ((uintptr_t) buffer + size > region->end);

    return 0;
}

int validate_kernel_read(const void *buffer, size_t size) {
    if (!buffer) {
        return -EFAULT;
    }

    uintptr_t offset = 0;
    struct vm_region *region;
    do {
        region = find_kernel_vm_region_by_addr((uintptr_t) buffer + offset);
        if (!region || (region->flags & VM_PROT_NONE)) {
            return -EFAULT;
        }
        offset = region->end - (uintptr_t) buffer;
    } while ((uintptr_t) buffer + size > region->end);

    return 0;
}

int validate_write_or_null(void *buffer, size_t size) {
    if (!buffer) {
        return 0;
    }

    return validate_write(buffer, size);
}

int validate_read_or_null(const void *buffer, size_t size) {
    if (!buffer) {
        return 0;
    }

    return validate_read(buffer, size);
}

int validate_signal_number(int signum, int unused) {
    (void) unused;

    if (signum < 0 || signum >= _NSIG) {
        return -EINVAL;
    }

    return 0;
}

int validate_positive(int n, int accept_zero) {
    if (accept_zero) {
        return n >= 0;
    } else {
        return n > 0;
    }
}
