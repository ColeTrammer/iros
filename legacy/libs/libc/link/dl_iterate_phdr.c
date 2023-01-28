#define __libc_internal

#include <bits/dynamic_elf_object.h>
#include <link.h>
#include <pthread.h>
#include <sys/iros.h>

int dl_iterate_phdr(int (*iter)(struct dl_phdr_info *info, size_t size, void *closure), void *closure) {
#ifdef __is_static
    struct dl_phdr_info executable_info = {
        .dlpi_addr = 0,
        .dlpi_name = "",
        .dlpi_phdr = __initial_process_info->program_phdr_start,
        .dlpi_phnum = __initial_process_info->program_phdr_count,
        .dlpi_tls_modid = 0,
        .dlpi_tls_data = __initial_process_info->tls_start,
    };
    return iter(&executable_info, sizeof(executable_info), closure);
#else
    return __loader_iter_phdr(iter, closure);
#endif /* __is_static */
}
