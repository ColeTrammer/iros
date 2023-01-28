#ifndef _LINK_H
#define _LINK_H 1

#include <bits/size_t.h>
#include <elf.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct dl_phdr_info {
    ElfW(Addr) dlpi_addr;
    const char *dlpi_name;
    const ElfW(Phdr) * dlpi_phdr;
    ElfW(Half) dlpi_phnum;

    size_t dlpi_tls_modid;
    void *dlpi_tls_data;
};

int dl_iterate_phdr(int (*iter)(struct dl_phdr_info *info, size_t size, void *closure), void *closure);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LINK_H */
