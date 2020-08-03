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

    int error = 0;
    struct mapped_elf_file mapped_file = __loader_build_mapped_elf_file(file, &error);
    if (error) {
        __dl_set_error("Not dynamic elf object `%s'", file);
        return ret;
    }

    struct dynamic_elf_object *object = __loader_load_mapped_elf_file(&mapped_file);
    if (!object) {
        __dl_set_error("Cannot load `%s'", file);
        goto cleanup_mapped_file;
    }

    __loader_add_dynamic_object(object);
    __loader_load_dependencies(object);
    __loader_process_relocations(object);
    __loader_call_init_functions(object, 0, NULL, NULL);
    ret = object;

cleanup_mapped_file:
    __loader_destroy_mapped_elf_file(&mapped_file);
    return ret;
}
