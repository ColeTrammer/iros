#include <bits/dynamic_elf_object.h>
#include <dlfcn.h>

int dladdr(void *addr, Dl_info *info) {
    struct symbol_lookup_result result = __loader_do_addr_lookup(addr);
    if (result.object == NULL) {
        return 0;
    }

    info->dli_fname = __loader_object_name(result.object);
    info->dli_fbase = (void *) result.object->relocation_offset;

    if (result.symbol) {
        info->dli_sname = __loader_dynamic_string(result.object, result.symbol->st_name);
        info->dli_saddr = (void *) (result.object->relocation_offset + result.symbol->st_value);
    } else {
        info->dli_sname = NULL;
        info->dli_saddr = NULL;
    }
    return 1;
}
