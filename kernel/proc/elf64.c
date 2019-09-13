#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/process.h>

bool elf64_is_valid(void *buffer) {
    /* Should Probably Also Check Sections And Architecture */

    Elf64_Ehdr *elf_header = buffer;
    return strcmp((char*) elf_header->e_ident, ELF64_MAGIC) == 0;
}

uintptr_t elf64_get_start(void *buffer) {
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Phdr *program_header = (Elf64_Phdr*) ((uintptr_t) buffer + elf_header->e_phoff);
    return program_header->p_vaddr;
}

uintptr_t elf64_get_entry(void *buffer) {
    Elf64_Ehdr *elf_header = buffer;
    return elf_header->e_entry;
}

uint64_t elf64_get_size(void *buffer) {
    uintptr_t start = elf64_get_start(buffer);
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Shdr *section_headers = (Elf64_Shdr*) (((uintptr_t) buffer) + elf_header->e_shoff);
    
    for (size_t i = 0; i < elf_header->e_shnum; i++) {
        if (section_headers[i].sh_addr && section_headers[i].sh_type == ELF64_NO_BITS) {
            return section_headers[i].sh_addr + section_headers[i].sh_size - start;
        }
    }
    return 0;
}

void elf64_load_program(void *buffer, size_t length, struct process *process) {
    uint64_t start = elf64_get_start(buffer);
    uint64_t size = elf64_get_size(buffer);
    uint64_t num_pages = NUM_PAGES(start, start + size);
    for (uint64_t i = 0; i < num_pages; i++) {
        map_page(start + i * PAGE_SIZE, VM_USER | VM_WRITE);
    }
    memcpy((void*) start, buffer, length);

    uint64_t types[4] = { VM_PROCESS_TEXT, VM_PROCESS_ROD, VM_PROCESS_DATA, VM_PROCESS_BSS };
    for (size_t i = 0; i < 4; i++) {
        uint64_t type = types[i];
        struct vm_region *region = elf64_create_vm_region(buffer, type);
        process->process_memory = add_vm_region(process->process_memory, region);
        map_vm_region_flags(region);

        if (type == VM_PROCESS_BSS) {
            memset((void*) region->start, 0, region->end - region->start);
        }
    }
}

void elf64_map_heap(void *buffer, struct process *process) {
    struct vm_region *process_heap = calloc(1, sizeof(struct vm_region));
    process_heap->flags = VM_USER | VM_WRITE | VM_NO_EXEC;
    process_heap->type = VM_PROCESS_HEAP;
    process_heap->start = ((elf64_get_start(buffer) + elf64_get_size(buffer)) & ~0xFFF) + PAGE_SIZE;
    process_heap->end = process_heap->start;
    process->process_memory = add_vm_region(process->process_memory, process_heap);
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
    region->start = section_headers[i].sh_addr & ~0xFF;
    region->end = ((section_headers[i].sh_addr + section_headers[i].sh_size) & ~0xFFF) + PAGE_SIZE;
    region->type = type;
    region->flags |= VM_USER;
    return region;
}