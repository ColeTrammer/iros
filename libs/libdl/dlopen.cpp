#define __libc_internal
#include <dlfcn.h>
#include <stdio.h>

#include "handle.h"

#define DL_LOG

extern "C" {
void* dlopen(const char* file, int flags) {
    (void) flags;
#ifdef DL_LOG
    fprintf(stderr, "trying to open file `%s' <LAZY=%d NOW=%d GLOBAL=%d LOCAL=%d>\n", file, !!(flags & RTLD_LAZY), !!(flags & RTLD_NOW),
            !!(flags & RTLD_GLOBAL), !!(flags & RTLD_LOCAL));
#endif /* DL_LOG */

    auto result = MappedElfFile::create(file);
    if (result.is<String>()) {
        __dl_set_error("%s", result.as<String>().string());
        return nullptr;
    }

    auto elf_file = move(result.get<0>());
    for (size_t i = 1; i < elf_file->section_count(); i++) {
        auto* section = elf_file->section_at(i);
        fprintf(stderr, "section \"%s\"\n", elf_file->section_string(section->sh_name));
    }

    for (size_t i = 0; i < elf_file->program_header_count(); i++) {
        auto* phdr = elf_file->program_header_at(i);
        fprintf(stderr, "program header type: %d\n", phdr->p_type);
    }

    auto* dynamic_table = elf_file->dynamic_table();
    if (!dynamic_table) {
        __dl_set_error("`%s' has no PT_DYNAMIC program header", file);
        return nullptr;
    }

    auto dynamic_object = DynamicElfObject(*elf_file);
    for (size_t i = 0; i < dynamic_object.rela_count(); i++) {
        auto* rela = dynamic_object.rela_at(i);
        auto type = ELF64_R_TYPE(rela->r_info);
        auto symbol_index = ELF64_R_SYM(rela->r_info);
        switch (type) {
                // A   - The addend used to compute the value of the relocatable field.
                // B   - The base address at which a shared object is loaded into memory during execution. Generally, a shared object file
                //       is built with a base virtual address of 0. However, the execution address of the shared object is different. See
                //       Program Header.
                // G   - The offset into the global offset table at which the address of the relocation entry's symbol resides during
                //       execution.
                // GOT - The address of the global offset table.
                // L   - The section offset or address of the procedure linkage table entry for a symbol.
                // P   - The section offset or address of the storage unit being relocated, computed using r_offset.
                // S   - The value of the symbol whose index resides in the relocation entry.
                // Z   - The size of the symbol whose index resides in the relocation entry.
            case R_X86_64_NONE:
                break;
            case R_X86_64_64:
                fprintf(stderr, "R_X86_64_64 for symbol %s\n", dynamic_object.symbol_name(symbol_index));
                break;
            case R_X86_64_RELATIVE: {
                // B + A
                auto B = reinterpret_cast<uintptr_t>(elf_file->data());
                auto A = rela->r_addend;
                auto* addr = reinterpret_cast<uint64_t*>(elf_file->data() + rela->r_offset);
                *addr = B + A;
                break;
            }
            default:
                fprintf(stderr, "Unkown relocation type %ld\n", type);
                break;
        }
    }

    return new Handle { move(elf_file), dynamic_object };
}
}
