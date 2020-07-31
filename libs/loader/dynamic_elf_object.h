#ifndef _DYNAMIC_ELF_OBJECT_H
#define _DYNAMIC_ELF_OBJECT_H 1

#include <stddef.h>
#include <stdint.h>

#include "loader.h"

struct Elf64_Dyn;
struct Elf64_Rela;
struct Elf64_Sym;
struct Elf64_Word;

struct dynamic_elf_object {
    struct dynamic_elf_object *next;
    struct dynamic_elf_object *prev;
    uintptr_t hash_table;
    uintptr_t string_table;
    uintptr_t symbol_table;
    uintptr_t preinit_array;
    size_t preinit_array_size;
    uintptr_t init_addr;
    uintptr_t init_array;
    uintptr_t init_array_size;
    uintptr_t fini_array;
    uintptr_t fini_array_size;
    uintptr_t fini_addr;
    uintptr_t rela_addr;
    uintptr_t plt_addr;
    size_t plt_size;
    size_t plt_type;
    uintptr_t got_addr;
    size_t rela_size;
    size_t rela_entry_size;
    size_t so_name_offset;
    size_t string_table_size;
    size_t symbol_entry_size;
    uint8_t *raw_data;
    size_t raw_data_size;
    uintptr_t relocation_offset;
    size_t *dependencies;
    size_t dependencies_size;
    size_t dependencies_max;
};

struct dynamic_elf_object build_dynamic_elf_object(const Elf64_Dyn *dynamic_table, size_t dynamic_count, uint8_t *base, size_t size,
                                                   size_t relocation_offset) LOADER_PRIVATE;
void destroy_dynamic_elf_object(struct dynamic_elf_object *self) LOADER_PRIVATE;
const char *dynamic_strings(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const char *dynamic_string(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
const char *object_name(const struct dynamic_elf_object *self) LOADER_PRIVATE;
size_t rela_count(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const Elf64_Rela *rela_table(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const Elf64_Rela *rela_at(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
size_t plt_relocation_count(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const void *plt_relocation_table(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const void *plt_relocation_at(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
const Elf64_Sym *symbol_table(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const Elf64_Sym *symbol_at(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
const char *symbol_name(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
const Elf64_Word *hash_table(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const Elf64_Sym *lookup_symbol(const struct dynamic_elf_object *self, const char *s) LOADER_PRIVATE;
void free_dynamic_elf_object(struct dynamic_elf_object *self) LOADER_PRIVATE;
void call_init_functions(struct dynamic_elf_object *obj, int argc, char **argv, char **envp) LOADER_PRIVATE;
void add_dynamic_object(struct dynamic_elf_object *obj) LOADER_PRIVATE;
void load_dependencies(struct dynamic_elf_object *obj) LOADER_PRIVATE;

#endif /* _DYNAMIC_ELF_OBJECT_H */
