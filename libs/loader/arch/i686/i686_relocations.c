#include <elf.h>

#include "../../dynamic_elf_object.h"
#include "../../relocations.h"
#include "../../symbols.h"
#include "../../tls_record.h"

static int do_rel(struct dynamic_elf_object *self, const Elf32_Rel *rel) {
    size_t type = ELF32_R_TYPE(rel->r_info);
    size_t symbol_index = ELF32_R_SYM(rel->r_info);
    uint32_t *addr = (uint32_t *) (self->relocation_offset + rel->r_offset);
    switch (type) {
            // A               - The addend used to compute the value of the relocatable field.
            // B               - The base address at which a shared object is loaded into memory during execution. Generally, a shared
            //                   object file is built with a base virtual address of 0. However, the execution address of the shared object
            //                   is different.
            // G               - The offset into the global offset table at which the address of the relocation entry's symbol resides
            //                   during execution.
            // GOT             - The address of the global offset table.
            // L               - The section offset or address of the procedure linkage table entry for a symbol.
            // P               - The section offset or address of the storage unit being relocated, computed using r_offset.
            // S               - The value of the symbol whose index resides in the relocation entry.
            // Z               - The size of the symbol whose index resides in the relocation entry.
            // @tlsgd(x)       - Allocates two contiguous entries in the GOT to hold a TLS_index structure. This structure is passed to
            //                   ___tls_get_addr(). The instruction referencing this entry will be bound to the first of the two GOT
            //                   entries.
            // @tlsgdplt(x)    - This relocation is handled as if it were a R_386_PLT32 relocation referencing the ___tls_get_addr()
            //                   function.
            // @tlsldm(x)      - Allocates two contiguous entries in the GOT to hold a TLS_index structure. This structure is passed to the
            //                   ___tls_get_addr(). Theti_tlsoffset field of the TLS_index is set to 0, and the ti_moduleid is filled in at
            //                   runtime. The call to ___tls_get_addr() returns the starting offset of the dynamic TLS block.
            // @gotntpoff(x)   - Allocates a entry in the GOT, and initializes it with the negative tlsoffset relative to the static TLS
            //                   block. This is performed at runtime via the R_386_TLS_TPOFF relocation.
            // @indntpoff(x)   - This expression is similar to @gotntpoff, but used in position dependent code. @gotntpoff resolves to a GOT
            //                   slot address relative to the start of the GOT in the movl or addl instructions. @indntpoff resolves to the
            //                   absolute GOT slot address.
            // @ntpoff(x)      - Calculates the negative offset of the variable it is added to relative to the static TLS block.
            // @dtpoff(x)      - Calculates the tlsoffset relative to the TLS block. The value is used as an immediate value of an addend
            //                   and is not associated with a specific register.
            // @dtpmod(x)      - Calculates the object identifier of the object containing symbol S.
        case R_386_NONE:
            break;
        case R_386_32: {
            // S + A
            const Elf32_Sym *symbol_to_lookup = symbol_at(self, symbol_index);
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            uintptr_t symbol_value = 0;
            if (result.symbol) {
                symbol_value = result.symbol->st_value + result.object->relocation_offset;
            } else if (!((symbol_to_lookup->st_info >> 4) & STB_WEAK)) {
                loader_err("Cannot resolve `%s' for `%s'", to_lookup, object_name(self));
                return -1;
            }

            uintptr_t A = *addr;
            *addr = symbol_value + A;
            break;
        }
        case R_386_PC32: {
            // S + A - P
            const Elf32_Sym *symbol_to_lookup = symbol_at(self, symbol_index);
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            uintptr_t symbol_value = 0;
            if (result.symbol) {
                symbol_value = result.symbol->st_value + result.object->relocation_offset;
            } else if (!((symbol_to_lookup->st_info >> 4) & STB_WEAK)) {
                loader_err("Cannot resolve `%s' for `%s'", to_lookup, object_name(self));
                return -1;
            }

            uintptr_t A = *addr;
            uintptr_t P = rel->r_offset;
            *addr = symbol_value + A - P;
            break;
        }
        case R_386_COPY: {
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, SYMBOL_LOOKUP_NOT_CURRENT);
            if (!result.symbol) {
                loader_err("Cannot resolve `%s' for `%s'", to_lookup, object_name(self));
                return -1;
            }
            const void *src = (const void *) (result.symbol->st_value + result.object->relocation_offset);
            memcpy(addr, src, result.symbol->st_size);
            break;
        }
        case R_386_GLOB_DAT:
        case R_386_JMP_SLOT: {
            // S
            const Elf32_Sym *symbol_to_lookup = symbol_at(self, symbol_index);
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            uintptr_t symbol_value = 0;
            if (result.symbol) {
                symbol_value = result.symbol->st_value + result.object->relocation_offset;
            } else if (!((symbol_to_lookup->st_info >> 4) & STB_WEAK)) {
                loader_err("Cannot resolve `%s' for `%s'", to_lookup, object_name(self));
                return -1;
            }

            *addr = symbol_value;
            break;
        }
        case R_386_RELATIVE: {
            // B + A
            uintptr_t A = *addr;
            uintptr_t B = (uintptr_t) self->relocation_offset;
            *addr = A + B;
            break;
        }
        case R_386_TLS_TPOFF: {
            // @ntpoff(S)
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            if (!result.symbol) {
                loader_err("Cannot resolve `%s' for `%s'", to_lookup, object_name(self));
                return -1;
            } else if (!result.object->tls_module_id || (result.symbol->st_info & 0xF) != STT_TLS) {
                loader_err("Found `%s' in `%s', but the symbol is not thread local", to_lookup, object_name(result.object));
                return -1;
            }

            // NOTE: for this to work, the symbol must be located in the initial thread local storage area.
            struct tls_record *record = &tls_records[result.object->tls_module_id - 1];
            *addr = -(record->tls_offset - result.symbol->st_value);

#ifdef LOADER_TLS_DEBUG
            loader_log("Resolved @ntpoff(`%s') to %#.8lX", to_lookup, record->tls_offset - result.symbol->st_value);
#endif /* LOADER_TLS_DEBUG */
            break;
        }
        case R_386_TLS_DTPMOD32: {
            // @dtpmod(s)

            // Static tls variables don't need a symbol index for relocation, they only need their own tls module id.
            if (symbol_index) {
                const char *to_lookup = symbol_name(self, symbol_index);
                struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
                if (!result.symbol) {
                    loader_err("Cannot resolve `%s' for `%s'", to_lookup, object_name(self));
                    return -1;
                } else if (!result.object->tls_module_id || (result.symbol->st_info & 0xF) != STT_TLS) {
                    loader_err("Found `%s' in `%s', but the symbol is not thread local", to_lookup, object_name(result.object));
                    return -1;
                }
                *addr = result.object->tls_module_id;
                break;
            }

            *addr = self->tls_module_id;
            break;
        }
        case R_386_TLS_DTPOFF32: {
            // @dtpoff(s)
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            if (!result.symbol) {
                loader_err("Cannot resolve `%s' for `%s'", to_lookup, object_name(self));
                return -1;
            } else if (!result.object->tls_module_id || (result.symbol->st_info & 0xF) != STT_TLS) {
                loader_err("Found `%s' in `%s', but the symbol is not thread local", to_lookup, object_name(result.object));
                return -1;
            }

            struct tls_record *record = &tls_records[result.object->tls_module_id - 1];
            *addr = record->tls_offset;

#ifdef LOADER_TLS_DEBUG
            loader_log("Resolved @dtpoff(`%s') to %#.8zX", to_lookup, record->tls_offset);
#endif /* LOADER_TLS_DEBUG */
            break;
        }
        default:
            loader_log("Unkown relocation type %ld", type);
            return -1;
    }

    return 0;
}

int do_process_relocations(struct dynamic_elf_object *self, bool bind_now) {
    if (self->was_relocated) {
        return 0;
    }
    self->was_relocated = true;

    size_t count = rel_count(self);
    for (size_t i = 0; i < count; i++) {
        const Elf32_Rel *rel = rel_at(self, i);
        if (do_rel(self, rel)) {
            return -1;
        }
    }

    size_t plt_count = plt_relocation_count(self);
    if (1 || bind_now) {
        for (size_t i = 0; i < plt_count; i++) {
            const Elf32_Rel *rel = plt_relocation_at(self, i);
            if (do_rel(self, rel)) {
                return -1;
            }
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

    return 0;
}
