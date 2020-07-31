#ifndef _MAPPED_ELF_FILE_H
#define _MAPPED_ELF_FILE_H 1

#include "loader.h"

struct Elf64_Ehdr;
struct Elf64_Phdr;
struct Elf64_Shdr;
struct dynamic_elf_object;

struct mapped_elf_file {
    void *base;
    size_t size;
};

struct mapped_elf_file build_mapped_elf_file(const char *file, int *error) LOADER_PRIVATE;
void destroy_mapped_elf_file(struct mapped_elf_file *self) LOADER_PRIVATE;
const Elf64_Ehdr *elf_header(const struct mapped_elf_file *self) LOADER_PRIVATE;
const Elf64_Shdr *section_table(const struct mapped_elf_file *self) LOADER_PRIVATE;
const Elf64_Shdr *section_at(const struct mapped_elf_file *self, size_t index) LOADER_PRIVATE;
size_t section_count(const struct mapped_elf_file *self) LOADER_PRIVATE;
const Elf64_Phdr *program_header_table(const struct mapped_elf_file *self) LOADER_PRIVATE;
const Elf64_Phdr *program_header_at(const struct mapped_elf_file *self, size_t index) LOADER_PRIVATE;
size_t program_header_count(const struct mapped_elf_file *self) LOADER_PRIVATE;
const char *section_strings(const struct mapped_elf_file *self) LOADER_PRIVATE;
const char *section_string(const struct mapped_elf_file *self, size_t index) LOADER_PRIVATE;
const char *strings(const struct mapped_elf_file *self) LOADER_PRIVATE;
const char *string(const struct mapped_elf_file *self, size_t index) LOADER_PRIVATE;
const Elf64_Phdr *dynamic_program_header(const struct mapped_elf_file *self) LOADER_PRIVATE;
uintptr_t dynamic_table_offset(const struct mapped_elf_file *self) LOADER_PRIVATE;
size_t dynamic_count(const struct mapped_elf_file *self) LOADER_PRIVATE;
struct dynamic_elf_object *load_mapped_elf_file(struct mapped_elf_file *file) LOADER_PRIVATE;

#endif /* _MAPPED_ELF_FILE_H */
