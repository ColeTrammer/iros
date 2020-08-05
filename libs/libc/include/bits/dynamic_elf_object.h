#ifndef _BITS_DYNAMIC_ELF_OBJECT_H
#define _BITS_DYNAMIC_ELF_OBJECT_H 1

#include <elf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct dynamic_elf_object;
struct tls_record;

union dynamic_elf_object_dependency {
    size_t string_table_offset;
    struct dynamic_elf_object *resolved_object;
};

struct dynamic_elf_object {
    struct dynamic_elf_object *next;
    struct dynamic_elf_object *prev;
    struct dynamic_elf_object *bfs_queue_next;
    struct tls_record *tls_record;
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
    union dynamic_elf_object_dependency *dependencies;
    size_t dependencies_size;
    size_t dependencies_max;
    bool global : 1;
    bool dependencies_were_loaded : 1;
    bool was_relocated : 1;
    bool init_functions_called : 1;
    bool fini_functions_called : 1;
};

void __loader_call_init_functions(struct dynamic_elf_object *obj, int argc, char **argv, char **envp) __attribute__((weak));
void __loader_call_fini_functions(struct dynamic_elf_object *obj) __attribute__((weak));
int __loader_process_relocations(struct dynamic_elf_object *self, bool bind_now) __attribute__((weak));
int __loader_load_dependencies(struct dynamic_elf_object *obj_head) __attribute__((weak));
const Elf64_Sym *__loader_lookup_symbol(const struct dynamic_elf_object *self, const char *s) __attribute__((weak));
void __loader_free_dynamic_elf_object(struct dynamic_elf_object *obj) __attribute__((weak));
struct dynamic_elf_object *__loader_get_dynamic_object_head(void) __attribute__((weak));
struct dynamic_elf_object *__loader_get_dynamic_object_tail(void) __attribute__((weak));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITS_DYNAMIC_ELF_OBJECT_H */
