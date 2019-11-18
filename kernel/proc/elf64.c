#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/param.h>

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
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Phdr *program_headers = (Elf64_Phdr*) (((uintptr_t) buffer) + elf_header->e_phoff);
    
    assert(elf_header->e_phnum == 2);
    return program_headers[1].p_memsz - program_headers[0].p_vaddr;
}

void elf64_load_program(void *buffer, size_t length, struct process *process) {
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Phdr *program_headers = (Elf64_Phdr*) (((uintptr_t) buffer) + elf_header->e_phoff);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        uintptr_t program_section_start = program_headers[i].p_vaddr;
        assert(program_section_start < ((uintptr_t) buffer) + length);

        uintptr_t program_section_end = program_section_start + program_headers[i].p_memsz;
        uint64_t program_flags = program_headers[i].p_flags == 0x5 ? (VM_USER) : (VM_NO_EXEC | VM_WRITE | VM_USER);

        debug_log("Program header: [ %#.16lX, %#.16lX, %#.16lX ]\n", program_headers[i].p_vaddr, program_headers[i].p_filesz, program_headers[i].p_memsz);

        struct vm_region *to_add = calloc(1, sizeof(struct vm_region));
        to_add->start = program_section_start & ~0xFFFULL;
        to_add->end = ((program_section_end + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
        to_add->flags = VM_WRITE | VM_USER;
        to_add->type = program_flags & VM_NO_EXEC ? VM_PROCESS_DATA : VM_PROCESS_TEXT;
        
        process->process_memory = add_vm_region(process->process_memory, to_add);

        map_vm_region(to_add);

        memcpy((char*) program_section_start, ((char*) buffer) + program_headers[i].p_offset, program_headers[i].p_filesz);
        memset((char*) program_section_start + program_headers[i].p_filesz, 0, program_headers[i].p_memsz - program_headers[i].p_filesz);

        to_add->flags = program_flags;
        map_vm_region_flags(to_add);
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