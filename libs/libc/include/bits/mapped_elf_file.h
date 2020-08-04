#ifndef _BITS_MAPPED_ELF_FILE_H
#define _BITS_MAPPED_ELF_FILE_H 1

#include <bits/size_t.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mapped_elf_file {
    void *base;
    size_t size;
    int fd;
};

struct mapped_elf_file __loader_build_mapped_elf_file(const char *file) __attribute__((weak));
void __loader_destroy_mapped_elf_file(struct mapped_elf_file *self) __attribute__((weak));
struct dynamic_elf_object *__loader_load_mapped_elf_file(struct mapped_elf_file *file, const char *name, bool global) __attribute__((weak));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITS_MAPPED_ELF_FILE_H */
