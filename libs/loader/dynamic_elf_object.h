#ifndef _DYNAMIC_ELF_OBJECT_H
#define _DYNAMIC_ELF_OBJECT_H 1

#include <bits/dynamic_elf_object.h>
#include <elf.h>
#include <stddef.h>
#include <stdint.h>

#include "loader.h"

struct tls_record;

struct dynamic_elf_object build_dynamic_elf_object(const ElfW(Dyn) * dynamic_table, size_t dynamic_count, uint8_t *base, size_t size,
                                                   size_t relocation_offset, void *phdr_start, size_t phdr_count, size_t tls_module_id,
                                                   const char *full_path, bool global) LOADER_PRIVATE;
void destroy_dynamic_elf_object(struct dynamic_elf_object *self) LOADER_PRIVATE;
void drop_dynamic_elf_object(struct dynamic_elf_object *self) LOADER_PRIVATE;
struct dynamic_elf_object *bump_dynamic_elf_object(struct dynamic_elf_object *self) LOADER_PRIVATE;
const char *dynamic_strings(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const char *dynamic_string(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
const char *object_name(const struct dynamic_elf_object *self) LOADER_PRIVATE;
size_t rela_count(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const ElfW(Rela) * rela_table(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const ElfW(Rela) * rela_at(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
size_t plt_relocation_count(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const void *plt_relocation_table(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const void *plt_relocation_at(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
const ElfW(Sym) * symbol_table(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const ElfW(Sym) * symbol_at(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
const char *symbol_name(const struct dynamic_elf_object *self, size_t i) LOADER_PRIVATE;
const ElfW(Word) * hash_table(const struct dynamic_elf_object *self) LOADER_PRIVATE;
const ElfW(Sym) * lookup_symbol(const struct dynamic_elf_object *self, const char *s) LOADER_PRIVATE;
const ElfW(Sym) * lookup_addr(const struct dynamic_elf_object *self, uintptr_t addr) LOADER_PRIVATE;
void free_dynamic_elf_object(struct dynamic_elf_object *self) LOADER_PRIVATE;
void call_init_functions(struct dynamic_elf_object *obj, int argc, char **argv, char **envp) LOADER_PRIVATE;
void call_fini_functions(struct dynamic_elf_object *obj) LOADER_PRIVATE;
void add_dynamic_object(struct dynamic_elf_object *obj) LOADER_PRIVATE;
void remove_dynamic_object(struct dynamic_elf_object *obj) LOADER_PRIVATE;
int load_dependencies(struct dynamic_elf_object *obj) LOADER_PRIVATE;
struct dynamic_elf_object *get_dynamic_object_head(void) LOADER_PRIVATE;
struct dynamic_elf_object *get_dynamic_object_tail(void) LOADER_PRIVATE;

#endif /* _DYNAMIC_ELF_OBJECT_H */
