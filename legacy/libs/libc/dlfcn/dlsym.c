#define __libc_internal
#include <assert.h>
#include <bits/dynamic_elf_object.h>
#include <dlfcn.h>
#include <elf.h>
#include <stdio.h>

void *dlsym(void *__restrict _handle, const char *__restrict symbol) {
    struct dynamic_elf_object *queue_head = _handle;
    struct dynamic_elf_object *queue_tail = _handle;

    if (queue_head->is_program) {
        struct symbol_lookup_result result = __loader_do_symbol_lookup(symbol, queue_head, 0);
        if (result.symbol == NULL) {
            goto error;
        }
        return (void *) (result.symbol->st_value + result.object->relocation_offset);
    }

    while (queue_head) {
        struct dynamic_elf_object *obj = queue_head;
        assert(obj->dependencies_were_loaded);
        queue_head = queue_head->bfs_queue_next;
        obj->bfs_queue_next = NULL;

        const ElfW(Sym) *sym = __loader_lookup_symbol(obj, symbol);
        if (sym && sym->st_shndx != SHN_UNDEF && (sym->st_info >> 4 == STB_GLOBAL || sym->st_info >> 4 == STB_WEAK)) {
            return (void *) (sym->st_value + obj->relocation_offset);
        }

        for (size_t i = 0; i < obj->dependencies_size; i++) {
            struct dynamic_elf_object *dependency = obj->dependencies[i].resolved_object;
            if (!queue_head) {
                dependency->bfs_queue_next = NULL;
                queue_head = queue_tail = dependency;
            } else {
                queue_tail->bfs_queue_next = dependency;
                queue_tail = dependency;
            }
        }
    }

error:
    __dl_set_error("could not find symbol `%s'", symbol);
    return NULL;
}
