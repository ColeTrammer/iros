#include <stdint.h>
#include <stdlib.h>

#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/elf64.h>

uintptr_t elf64_get_start(void *buffer) {
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Phdr *program_header = (Elf64_Phdr*) ((uintptr_t) buffer + elf_header->e_phoff);
    return program_header->p_vaddr;
}

uintptr_t elf64_get_entry(void *buffer) {
    Elf64_Ehdr *elf_header = buffer;
    return elf_header->e_entry;
}

struct vm_region *elf64_create_vm_region(void *buffer, uint64_t type) {
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Shdr *section_headers = (Elf64_Shdr*) (((uintptr_t) buffer) + elf_header->e_shoff);

    struct vm_region *region = calloc(1, sizeof(struct vm_region));

    size_t i;
    for (i = 0; i < elf_header->e_shnum; i++) {
        if (section_headers[i].sh_addr) {
            switch (type) {
                case VM_PROCESS_TEXT: 
                    if (section_headers[i].sh_flags == (ELF64_EXEC | ELF64_ALLOC)) {
                        goto found_section;
                    }
                    break;
                case VM_PROCESS_ROD: 
                    if (section_headers[i].sh_flags == ELF64_ALLOC) {
                        region->flags = VM_NO_EXEC;
                        goto found_section;
                    }
                    break;
                case VM_PROCESS_DATA: 
                    if (section_headers[i].sh_type == ELF64_PROGRAM_BITS && section_headers[i].sh_flags == (ELF64_WRITE | ELF64_ALLOC)) {
                        region->flags = VM_NO_EXEC | VM_WRITE;
                        goto found_section;
                    }
                    break;
                case VM_PROCESS_BSS: 
                    if (section_headers[i].sh_type == ELF64_NO_BITS) {
                        region->flags = VM_NO_EXEC | VM_WRITE;
                        goto found_section;
                    }
                    break;
            }
        }
    }

    return NULL;

    found_section:
    region->start = section_headers[i].sh_addr;
    region->end = ((section_headers[i].sh_addr + section_headers[i].sh_size) & ~0xFFF) + PAGE_SIZE;
    region->type = type;
    region->flags |= VM_USER;
    return region;
}