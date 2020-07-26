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

    auto loaded_elf_result = LoadedElfExecutable::create(*elf_file);
    if (loaded_elf_result.is<String>()) {
        __dl_set_error("%s", loaded_elf_result.as<String>().string());
        return nullptr;
    }

    auto loaded_elf = move(loaded_elf_result.get<0>());
    auto dynamic_object = DynamicElfObject(*elf_file, loaded_elf->base());
    dynamic_object.process_relocations();
    return new Handle { move(loaded_elf), dynamic_object };
}
}
