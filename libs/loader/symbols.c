#include <elf.h>

#include "dynamic_elf_object.h"
#include "symbols.h"

struct symbol_lookup_result do_symbol_lookup(const char *s, const struct dynamic_elf_object *current_object, int flags) {
    struct dynamic_elf_object *obj = dynamic_object_head;
#ifdef LOADER_SYMBOL_DEBUG
    loader_log("looking up `%s' for `%s'", s, object_name(current_object));
#endif /* LOADER_SYMBOL_DEBUG */
    while (obj) {
        if (!(flags & SYMBOL_LOOKUP_NOT_CURRENT) || (obj != current_object)) {
            const Elf64_Sym *sym = lookup_symbol(obj, s);
            if (sym && sym->st_shndx != STN_UNDEF) {
                uint8_t visibility = sym->st_info >> 4;
                if (visibility == STB_GLOBAL || visibility == STB_WEAK) {
                    return (struct symbol_lookup_result) { .symbol = sym, .object = obj };
                }
            }
        }
        obj = obj->next;
    }
    return (struct symbol_lookup_result) { .symbol = NULL, .object = NULL };
}
