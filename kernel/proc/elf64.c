#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/fs/inode_store.h>
#include <kernel/fs/vfs.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/task.h>

// #define ELF64_DEBUG

bool elf64_is_valid(void *buffer) {
    /* Should Probably Also Check Sections And Architecture */

    Elf64_Ehdr *elf_header = buffer;
    return strcmp((char *) elf_header->e_ident, ELF64_MAGIC) == 0;
}

uintptr_t elf64_get_start(void *buffer) {
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Phdr *program_header = (Elf64_Phdr *) ((uintptr_t) buffer + elf_header->e_phoff);
    return program_header->p_vaddr;
}

uintptr_t elf64_get_entry(void *buffer) {
    Elf64_Ehdr *elf_header = buffer;
    return elf_header->e_entry;
}

uint64_t elf64_get_size(void *buffer) {
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Phdr *program_headers = (Elf64_Phdr *) (((uintptr_t) buffer) + elf_header->e_phoff);

    assert(elf_header->e_phnum >= 2);
    return program_headers[1].p_memsz + program_headers[0].p_filesz;
}

void elf64_load_program(void *buffer, size_t length, struct task *task) {
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Phdr *program_headers = (Elf64_Phdr *) (((uintptr_t) buffer) + elf_header->e_phoff);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        uintptr_t program_section_start = program_headers[i].p_vaddr;
        if (program_section_start == 0) {
            continue;
        }

        assert(program_section_start < ((uintptr_t) buffer) + length);

        uintptr_t program_section_end = program_section_start + program_headers[i].p_memsz;
        uint64_t program_flags = program_headers[i].p_flags == 0x5 ? (VM_USER) : (VM_NO_EXEC | VM_WRITE | VM_USER);

        struct vm_region *to_add = calloc(1, sizeof(struct vm_region));
        to_add->start = program_section_start & ~0xFFFULL;
        to_add->end = ((program_section_end + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
        to_add->flags = VM_WRITE | VM_USER;
        to_add->type = program_flags & VM_NO_EXEC ? VM_PROCESS_DATA : VM_PROCESS_TEXT;

        task->process->process_memory = add_vm_region(task->process->process_memory, to_add);

        map_vm_region(to_add);

        memcpy((char *) program_section_start, ((char *) buffer) + program_headers[i].p_offset, program_headers[i].p_filesz);
        memset((char *) program_section_start + program_headers[i].p_filesz, 0, program_headers[i].p_memsz - program_headers[i].p_filesz);

        to_add->flags = program_flags;
        map_vm_region_flags(to_add);
    }
}

void elf64_map_heap(void *buffer, struct task *task) {
    struct vm_region *task_heap = calloc(1, sizeof(struct vm_region));
    task_heap->flags = VM_USER | VM_WRITE | VM_NO_EXEC;
    task_heap->type = VM_PROCESS_HEAP;
    task_heap->start = ((elf64_get_start(buffer) + elf64_get_size(buffer)) & ~0xFFF) + 100 * PAGE_SIZE;
#ifdef ELF64_DEBUG
    debug_log("Heap start: [ %#.16lX ]\n", task_heap->start);
#endif /* ELF64_DEBUG */
    task_heap->end = task_heap->start;
    task->process->process_memory = add_vm_region(task->process->process_memory, task_heap);
}

// NOTE: this must be called from within a task's address space
void elf64_stack_trace(struct task *task) {
    struct inode *inode = fs_inode_get(task->process->inode_dev, task->process->inode_id);
    if (!inode) {
        debug_log("The task has no inode: [ %d, %lu, %llu ]\n", task->process->pid, task->process->inode_dev, task->process->inode_id);
        return;
    }

    int error = 0;
    struct file *file = inode->i_op->open(inode, O_RDONLY, &error);
    if (!file || error != 0) {
        return;
    }

    void *buffer = malloc(inode->size);
    fs_read(file, buffer, inode->size);
    free(file);

    if (!elf64_is_valid(buffer)) {
        debug_log("The task is not elf64: [ %d ]\n", task->process->pid);
        return;
    }

    Elf64_Ehdr *elf_header = (Elf64_Ehdr *) buffer;
    Elf64_Shdr *section_headers = (Elf64_Shdr *) (((uintptr_t) buffer) + elf_header->e_shoff);

    Elf64_Sym *symbols = NULL;
    uintptr_t symbols_size = 0;
    char *section_string_table = (char *) (((uintptr_t) buffer) + section_headers[elf_header->e_shstrndx].sh_offset);
    char *string_table = NULL;
    for (int i = 0; i < elf_header->e_shnum; i++) {
        // Symbol table
        if (section_headers[i].sh_type == 2) {
            symbols = (Elf64_Sym *) (((uintptr_t) buffer) + section_headers[i].sh_offset);
            symbols_size = section_headers[i].sh_size;
        } else if (section_headers[i].sh_type == 3 && strcmp(".strtab", section_string_table + section_headers[i].sh_name) == 0) {
            string_table = (char *) (((uintptr_t) buffer) + section_headers[i].sh_offset);
        }
    }

    assert(symbols);
    assert(string_table);

    uintptr_t rsp = (task->in_kernel || task->can_send_self_signals) ? task->arch_task.user_task_state.stack_state.rsp
                                                                     : task->arch_task.task_state.stack_state.rsp;
    uintptr_t rip = (task->in_kernel || task->can_send_self_signals) ? task->arch_task.user_task_state.stack_state.rip
                                                                     : task->arch_task.task_state.stack_state.rip;

    debug_log("Dumping core: [ %#.16lX, %#.16lX ]\n", rip, rsp);

    for (int i = 0; (uintptr_t)(symbols + i) < ((uintptr_t) symbols) + symbols_size; i++) {
        if (symbols[i].st_name != 0 && symbols[i].st_info == 18) {
            if (rip >= symbols[i].st_value && rip < symbols[i].st_value + symbols[i].st_size) {
                debug_log("[ %s ]\n", string_table + symbols[i].st_name);
            }
        }
    }

    for (rsp &= ~0xF; rsp < find_first_kernel_vm_region()->start - PAGE_SIZE; rsp += sizeof(uintptr_t)) {
        for (int i = 0; (uintptr_t)(symbols + i) < ((uintptr_t) symbols) + symbols_size; i++) {
            if (symbols[i].st_name != 0 && symbols[i].st_info == 18) {
                if (*((uint64_t *) rsp) >= symbols[i].st_value && *((uint64_t *) rsp) <= symbols[i].st_value + symbols[i].st_size) {
                    debug_log("[ %#.16lX, %s ]\n", symbols[i].st_value, string_table + symbols[i].st_name);
                }
            }
        }
    }

    free(buffer);
}