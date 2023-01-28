#include <link.h>
#include <stddef.h>
#include <stdio.h>

int main() {
    return dl_iterate_phdr(
        [](auto* info, auto, auto) {
#ifdef __x86_64__
            printf("Name `%s' (%d phdrs) [%#.16lX]\n", info->dlpi_name, info->dlpi_phnum, info->dlpi_addr);
#elif defined(__i386__)
            printf("Name `%s' (%d phdrs) [%#.8X]\n", info->dlpi_name, info->dlpi_phnum, info->dlpi_addr);
#endif
            for (int i = 0; i < info->dlpi_phnum; i++) {
                auto* phdr = &info->dlpi_phdr[i];
                const char* type = (phdr->p_type == PT_LOAD)      ? "PT_LOAD"
                                   : (phdr->p_type == PT_DYNAMIC) ? "PT_DYNAMIC"
                                   : (phdr->p_type == PT_INTERP)  ? "PT_INTERP"
                                   : (phdr->p_type == PT_NOTE)    ? "PT_NOTE"
                                   : (phdr->p_type == PT_INTERP)  ? "PT_INTERP"
                                   : (phdr->p_type == PT_PHDR)    ? "PT_PHDR"
                                   : (phdr->p_type == PT_TLS)     ? "PT_TLS"
                                                                  : "UNKNOWN";
#ifdef __x86_64__
                printf("  <%d>: [%#.16lX] (%12lu) %#X; %s\n", i, info->dlpi_addr + phdr->p_vaddr, phdr->p_memsz, phdr->p_flags, type);
#elif defined(__i386__)
                printf("  <%d>: [%#.16X] (%12u) %#X; %s\n", i, info->dlpi_addr + phdr->p_vaddr, phdr->p_memsz, phdr->p_flags, type);
#endif
            }
            return 0;
        },
        nullptr);
}
