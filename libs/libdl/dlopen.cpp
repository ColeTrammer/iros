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
            case R_X86_64_NONE:
                break;
            case R_X86_64_64:
                fprintf(stderr, "R_X86_64_64 for symbol %s\n", dynamic_object.symbol_name(symbol_index));
                break;
        }
    }

    return new Handle { move(elf_file), dynamic_object };
}
}
