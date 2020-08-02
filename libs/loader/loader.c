#include <elf.h>

#include "dynamic_elf_object.h"
#include "loader.h"
#include "mapped_elf_file.h"
#include "relocations.h"
#include "tls_record.h"

struct dynamic_elf_object *dynamic_object_head;
struct dynamic_elf_object *dynamic_object_tail;
const char *program_name;
bool bind_now;
__attribute__((nocommon)) struct initial_process_info *initial_process_info;
LOADER_HIDDEN_EXPORT(initial_process_info, __initial_process_info);

void LOADER_PRIVATE _entry(struct initial_process_info *info, int argc, char **argv, char **envp) {
    initial_process_info = info;
    program_name = *argv;

    struct tls_record *program_tls = NULL;
    if (info->tls_size) {
        program_tls = add_tls_record(info->tls_start, info->tls_size, info->tls_alignment, TLS_RECORD_PROGRAM);
    }
    struct dynamic_elf_object program =
        build_dynamic_elf_object((const Elf64_Dyn *) info->program_dynamic_start, info->program_dynamic_size / sizeof(Elf64_Dyn),
                                 (uint8_t *) info->program_offset, info->program_size, 0, program_tls);
    dynamic_object_head = dynamic_object_tail = &program;

    for (struct dynamic_elf_object *obj = &program; obj; obj = obj->next) {
        load_dependencies(obj);
    }

    struct dynamic_elf_object loader =
        build_dynamic_elf_object((const Elf64_Dyn *) info->loader_dynamic_start, info->loader_dynamic_size / sizeof(Elf64_Dyn),
                                 (uint8_t *) info->loader_offset, info->loader_size, info->loader_offset, NULL);
    add_dynamic_object(&loader);

    struct dynamic_elf_object *obj = dynamic_object_tail;
    while (obj) {
        process_relocations(obj);
        obj = obj->prev;
    }

    obj = dynamic_object_tail;
    while (obj) {
        call_init_functions(obj, argc, argv, envp);
        obj = obj->prev;
    }

#ifdef LOADER_DEBUG
    loader_log("starting program");
#endif /* LOADER_DEBUG */
    asm volatile("and $(~16), %%rsp\n"
                 "sub $8, %%rsp\n"
                 "mov %0, %%rdi\n"
                 "mov %1, %%esi\n"
                 "mov %2, %%rdx\n"
                 "mov %3, %%rcx\n"
                 "jmp *%4\n"
                 :
                 : "r"(info), "r"(argc), "r"(argv), "r"(envp), "r"(info->program_entry)
                 : "rdi", "rsi", "rdx", "rcx", "memory");
    _exit(99);
}
