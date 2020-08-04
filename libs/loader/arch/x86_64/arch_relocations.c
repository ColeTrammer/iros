#include <elf.h>

#include "../../dynamic_elf_object.h"
#include "../../relocations.h"
#include "../../symbols.h"
#include "../../tls_record.h"

static int do_rela(const struct dynamic_elf_object *self, const Elf64_Rela *rela) {
    size_t type = ELF64_R_TYPE(rela->r_info);
    size_t symbol_index = ELF64_R_SYM(rela->r_info);
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
            // @tlsgd(%rip)    - Allocates two contiguous entries in the GOT to hold a TLS_index structure.This structure is passed to
            //                   __tls_get_addr(). This instruction can only be used in the exact general dynamic code sequence.
            // @tlsld(%rip)    - Allocates two contiguous entries in the GOT to hold a TLS_index structure.This structure is passed to
            //                   __tls_get_addr(). At runtime, the ti_offset offset field of the object is set to zero, and the ti_module
            //                   offset is initialized. A call to the __tls_get_addr() function returns the starting offset if the dynamic
            //                   TLS block. This instruction can be used in the exact code sequence.
            // @dtpoff         - Calculates the offset of the variable relative to the start of the TLS block which contains the
            //                   variable. The computed value is used as an immediate value of an addend, and is not associated with a
            //                   specific register.
            // @dtpmod(x)      - Calculates the object identifier of the object containing a TLS symbol.
            // @gottpoff(%rip) - Allocates a entry in the GOT, to hold a variable offset in the initial TLS block. This offset is
            //                   relative to the TLS blocks end, %fs:0. The operator can only be used with a movq or addq instruction.
            // @tpoff(x)       - Calculates the offset of a variable relative to the TLS block end, %fs:0. No GOT entry is created.
        case R_X86_64_NONE:
            break;
        case R_X86_64_64: {
            // S + A
            const Elf64_Sym *symbol_to_lookup = symbol_at(self, symbol_index);
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            uintptr_t symbol_value = 0;
            if (result.symbol) {
                symbol_value = result.symbol->st_value + result.object->relocation_offset;
            } else if (!((symbol_to_lookup->st_info >> 4) & STB_WEAK)) {
                loader_err("Cannot resolve `%s'", to_lookup);
                return -1;
            }

            uint64_t *addr = (uint64_t *) (rela->r_offset + self->relocation_offset);
            *addr = symbol_value + rela->r_addend;
            break;
        }
        case R_X86_64_COPY: {
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, SYMBOL_LOOKUP_NOT_CURRENT);
            if (!result.symbol) {
                loader_err("Cannot resolve `%s'", to_lookup);
                return -1;
            }
            void *dest = (void *) (self->relocation_offset + rela->r_offset);
            const void *src = (const void *) (result.symbol->st_value + result.object->relocation_offset);
            memcpy(dest, src, result.symbol->st_size);
            break;
        }
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT: {
            // S
            const Elf64_Sym *symbol_to_lookup = symbol_at(self, symbol_index);
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            uintptr_t symbol_value = 0;
            if (result.symbol) {
                symbol_value = result.symbol->st_value + result.object->relocation_offset;
            } else if (!((symbol_to_lookup->st_info >> 4) & STB_WEAK)) {
                loader_err("Cannot resolve `%s'", to_lookup);
                return -1;
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
        case R_X86_64_DPTMOD64: {
            // @dtpmod(s)
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            if (!result.symbol) {
                loader_err("Cannot resolve `%s'", to_lookup);
                return -1;
            } else if (!result.object->tls_record || (result.symbol->st_info & 0xF) != STT_TLS) {
                loader_err("Found `%s' in `%s', but the symbol is not thread local", to_lookup, object_name(result.object));
                return -1;
            }
            uint64_t *addr = (uint64_t *) (self->relocation_offset + rela->r_offset);
            *addr = result.object->tls_record->tls_module_id;
            break;
        }
        case R_X86_64_DTPOFF64: {
            // @dtpoff(s)
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            if (!result.symbol) {
                loader_err("Cannot resolve `%s'", to_lookup);
                return -1;
            } else if (!result.object->tls_record || (result.symbol->st_info & 0xF) != STT_TLS) {
                loader_err("Found `%s' in `%s', but the symbol is not thread local", to_lookup, object_name(result.object));
                return -1;
            }
            uint64_t *addr = (uint64_t *) (self->relocation_offset + rela->r_offset);
            *addr = result.symbol->st_value;
            break;
        }
        case R_X86_64_TPOFF64: {
            // @tpoff(s)
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            if (!result.symbol) {
                loader_err("Cannot resolve `%s'", to_lookup);
                return -1;
            } else if (!result.object->tls_record || (result.symbol->st_info & 0xF) != STT_TLS) {
                loader_err("Found `%s' in `%s', but the symbol is not thread local", to_lookup, object_name(result.object));
                return -1;
            }

#ifdef LOADER_TLS_DEBUG
            loader_log("Resolved @tpoff(`%s') to %#.16lX", to_lookup, result.object->tls_record->tls_offset - result.symbol->st_value);
#endif /* LOADER_TLS_DEBUG */

            // NOTE: for this to work, the symbol must be located in the initial thread local storage area.
            uint64_t *addr = (uint64_t *) (self->relocation_offset + rela->r_offset);
            *addr = -(result.object->tls_record->tls_offset - result.symbol->st_value);
            break;
        }
        default:
            loader_log("Unkown relocation type %ld", type);
            return -1;
    }

    return 0;
}

int do_process_relocations(const struct dynamic_elf_object *self) {
    size_t count = rela_count(self);
    for (size_t i = 0; i < count; i++) {
        const Elf64_Rela *rela = rela_at(self, i);
        if (do_rela(self, rela)) {
            return -1;
        }
    }

    size_t plt_count = plt_relocation_count(self);
    if (bind_now) {
        for (size_t i = 0; i < plt_count; i++) {
            const Elf64_Rela *rela = plt_relocation_at(self, i);
            if (do_rela(self, rela)) {
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
