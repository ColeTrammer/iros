#define __libc_internal
#include <bits/dynamic_elf_object.h>
#include <bits/mapped_elf_file.h>
#include <dlfcn.h>
#include <stdio.h>

#define DL_LOG

void *dlopen(const char *file, int flags) {
    (void) flags;
#ifdef DL_LOG
    fprintf(stderr, "opening file `%s' <LAZY=%d NOW=%d GLOBAL=%d LOCAL=%d>\n", file, !!(flags & RTLD_LAZY), !!(flags & RTLD_NOW),
            !!(flags & RTLD_GLOBAL), !!(flags & RTLD_LOCAL));
#endif /* DL_LOG */

    struct dynamic_elf_object *ret = NULL;

    struct mapped_elf_file mapped_file = __loader_build_mapped_elf_file(file);
    if (mapped_file.base == NULL) {
        return ret;
    }

    struct dynamic_elf_object *object = __loader_load_mapped_elf_file(&mapped_file, file);
    if (!object) {
        goto cleanup_mapped_file;
    }

    if (__loader_load_dependencies(object)) {
        goto cleanup_dynamic_object;
    }

    if (__loader_process_relocations(object)) {
        goto cleanup_dynamic_object;
    }

    __loader_call_init_functions(object, 0, NULL, NULL);

    ret = object;
    goto cleanup_mapped_file;

cleanup_dynamic_object:
    __loader_free_dynamic_elf_object(object);

cleanup_mapped_file:
    __loader_destroy_mapped_elf_file(&mapped_file);
    return ret;
}
