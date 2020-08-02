#ifndef _DYNAMIC_ELF_OBJECT_H
#define _DYNAMIC_ELF_OBJECT_H 1

#include <bits/dynamic_elf_object.h>
#include <stddef.h>
#include <stdint.h>

#include "loader.h"

struct Elf64_Dyn;
struct Elf64_Rela;
struct Elf64_Sym;
struct Elf64_Word;
struct tls_record;

struct dynamic_elf_object build_dynamic_elf_object(const Elf64_Dyn *dynamic_table, size_t dynamic_count, uint8_t *base, size_t size,
                                                   size_t relocation_offset, struct tls_record *tls_record) LOADER_PRIVATE;
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
void call_fini_functions(struct dynamic_elf_object *obj) LOADER_PRIVATE;
void add_dynamic_object(struct dynamic_elf_object *obj) LOADER_PRIVATE;
void load_dependencies(struct dynamic_elf_object *obj) LOADER_PRIVATE;
struct dynamic_elf_object *get_dynamic_object_head(void) LOADER_PRIVATE;
struct dynamic_elf_object *get_dynamic_object_tail(void) LOADER_PRIVATE;

#endif /* _DYNAMIC_ELF_OBJECT_H */
