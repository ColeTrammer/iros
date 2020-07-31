#include <elf.h>

#include "dynamic_elf_object.h"
#include "relocations.h"
#include "symbols.h"

extern void got_resolver(void) LOADER_PRIVATE;

LOADER_PRIVATE uintptr_t do_got_resolve(const struct dynamic_elf_object *obj, size_t plt_offset) {
    const Elf64_Rela *relocation = plt_relocation_at(obj, plt_offset);
    const char *to_lookup = symbol_name(obj, ELF64_R_SYM(relocation->r_info));
    struct symbol_lookup_result result = do_symbol_lookup(to_lookup, obj, 0);
    if (!result.symbol) {
        loader_log("Cannot resolve `%s' for `%s'", to_lookup, object_name(obj));
        _exit(96);
    }
    uint64_t *addr = (uint64_t *) (obj->relocation_offset + relocation->r_offset);
    uintptr_t resolved_value = result.symbol->st_value + result.object->relocation_offset;
    *addr = resolved_value;
    return resolved_value;
}

static void do_rela(const struct dynamic_elf_object *self, const Elf64_Rela *rela) {
    size_t type = ELF64_R_TYPE(rela->r_info);
    size_t symbol_index = ELF64_R_SYM(rela->r_info);
    switch (type) {
            // A   - The addend used to compute the value of the relocatable field.
            // B   - The base address at which a shared object is loaded into memory during execution. Generally, a shared object
            // file
            //       is built with a base virtual address of 0. However, the execution address of the shared object is different.
            //       See Program Header.
            // G   - The offset into the global offset table at which the address of the relocation entry's symbol resides during
            //       execution.
            // GOT - The address of the global offset table.
            // L   - The section offset or address of the procedure linkage table entry for a symbol.
            // P   - The section offset or address of the storage unit being relocated, computed using r_offset.
            // S   - The value of the symbol whose index resides in the relocation entry.
            // Z   - The size of the symbol whose index resides in the relocation entry.
        case R_X86_64_NONE:
            break;
        case R_X86_64_64: {
            const Elf64_Sym *symbol_to_lookup = symbol_at(self, symbol_index);
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            uintptr_t symbol_value = 0;
            if (result.symbol) {
                symbol_value = result.symbol->st_value + result.object->relocation_offset;
            } else if (!((symbol_to_lookup->st_info >> 4) & STB_WEAK)) {
                loader_log("Cannot resolve `%s'", to_lookup);
                _exit(97);
            }

            uint64_t *addr = (uint64_t *) (rela->r_offset + self->relocation_offset);
            *addr = symbol_value + rela->r_addend;
            break;
        }
        case R_X86_64_COPY: {
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, SYMBOL_LOOKUP_NOT_CURRENT);
            if (!result.symbol) {
                loader_log("Cannot resolve `%s'", to_lookup);
                _exit(97);
            }
            void *dest = (void *) (self->relocation_offset + rela->r_offset);
            const void *src = (const void *) (result.symbol->st_value + result.object->relocation_offset);
            memcpy(dest, src, result.symbol->st_size);
            break;
        }
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT: {
            const Elf64_Sym *symbol_to_lookup = symbol_at(self, symbol_index);
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            uintptr_t symbol_value = 0;
            if (result.symbol) {
                symbol_value = result.symbol->st_value + result.object->relocation_offset;
            } else if (!((symbol_to_lookup->st_info >> 4) & STB_WEAK)) {
                loader_log("Cannot resolve `%s'", to_lookup);
                _exit(97);
            }

            uint64_t *addr = (uint64_t *) (rela->r_offset + self->relocation_offset);
            *addr = symbol_value;
            break;
        }
        case R_X86_64_RELATIVE: {
            // B + A
            uintptr_t B = (uintptr_t) self->relocation_offset;
            uintptr_t A = rela->r_addend;
            uint64_t *addr = (uint64_t *) (self->relocation_offset + rela->r_offset);
            *addr = B + A;
            break;
        }
        default:
            loader_log("Unkown relocation type %ld", type);
            break;
    }
}

void process_relocations(const struct dynamic_elf_object *self) {
    size_t count = rela_count(self);
    for (size_t i = 0; i < count; i++) {
        const Elf64_Rela *rela = rela_at(self, i);
        do_rela(self, rela);
    }

    size_t plt_count = plt_relocation_count(self);
    if (bind_now) {
        for (size_t i = 0; i < plt_count; i++) {
            const Elf64_Rela *rela = plt_relocation_at(self, i);
            do_rela(self, rela);
        }
    } else if (self->got_addr) {
        uintptr_t *got = (uintptr_t *) (self->got_addr + self->relocation_offset);
        got[1] = (uintptr_t) self;
        got[2] = (uintptr_t) &got_resolver;

        if (self->relocation_offset) {
            for (size_t i = 0; i < plt_count; i++) {
                got[3 + i] += self->relocation_offset;
            }
        }
    }
}
