#include <bits/dynamic_elf_object.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <stdio.h>

void dump_backtrace(void *const *buffer, int buffer_size) {
    for (int i = 0; i < buffer_size; i++) {
        void *addr = buffer[i];
        void *base_addr = addr;
        const char *library_name = "??";
        const char *symbol_name = "??";

#ifdef __is_shared
        struct symbol_lookup_result result = __loader_do_addr_lookup(addr);
        if (result.object != NULL) {
            library_name = __loader_object_name(result.object);
            base_addr -= result.object->relocation_offset;
            if (result.symbol) {
                symbol_name = __loader_dynamic_string(result.object, result.symbol->st_name);
            }
        }
#endif /* __is_shared */

        fprintf(stderr, "<%p %p>: %-20.20s %s\n", addr, base_addr, library_name, symbol_name);
    }
}
