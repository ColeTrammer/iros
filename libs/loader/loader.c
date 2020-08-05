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
bool ran_program;
__attribute__((nocommon)) struct initial_process_info *initial_process_info;
LOADER_HIDDEN_EXPORT(initial_process_info, __initial_process_info);

__attribute__((noreturn)) void _entry(struct initial_process_info *info, int argc, char **argv, char **envp) {
    initial_process_info = info;
    program_name = *argv;

    size_t program_tls_module_id = 0;
    if (info->tls_size) {
        program_tls_module_id =
            add_tls_record(info->tls_start, info->tls_size, info->tls_alignment, TLS_RECORD_PROGRAM | TLS_INITIAL_IMAGE);
    }
    struct dynamic_elf_object program =
        build_dynamic_elf_object((const Elf64_Dyn *) info->program_dynamic_start, info->program_dynamic_size / sizeof(Elf64_Dyn),
                                 (uint8_t *) info->program_offset, info->program_size, 0, program_tls_module_id, true);
    program.is_program = true; /* signals this object is the global handle used by dlopen(NULL). */
    add_dynamic_object(&program);
    if (load_dependencies(&program)) {
        _exit(99);
    }

    struct dynamic_elf_object loader =
        build_dynamic_elf_object((const Elf64_Dyn *) info->loader_dynamic_start, info->loader_dynamic_size / sizeof(Elf64_Dyn),
                                 (uint8_t *) info->loader_offset, info->loader_size, info->loader_offset, 0, true);
    add_dynamic_object(&loader);
    loader.dependencies_were_loaded = true; /* The loader cannot have any dependencies. */

    bool bind_now = false;
    if (process_relocations(&program, bind_now)) {
        _exit(98);
    }
    call_init_functions(&program, argc, argv, envp);

    loader_exec(info, argc, argv, envp);
}
