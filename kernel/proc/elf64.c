#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>

#include <kernel/fs/vfs.h>
#include <kernel/mem/anon_vm_object.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/task.h>
#include <kernel/util/validators.h>

// #define ELF64_DEBUG

static Elf64_Sym *kernel_symbols;
static char *kernel_string_table;
static size_t kernel_symbols_size;
static void *kernel_buffer;

static void try_load_symbols(void *buffer, Elf64_Sym **symbols, size_t *symbols_size, char **string_table) {
    Elf64_Ehdr *elf_header = (Elf64_Ehdr *) buffer;
    Elf64_Shdr *section_headers = (Elf64_Shdr *) (((uintptr_t) buffer) + elf_header->e_shoff);

    char *section_string_table = (char *) (((uintptr_t) buffer) + section_headers[elf_header->e_shstrndx].sh_offset);
    for (int i = 0; i < elf_header->e_shnum; i++) {
        // Symbol table
        if (section_headers[i].sh_type == 2) {
            *symbols = (Elf64_Sym *) (((uintptr_t) buffer) + section_headers[i].sh_offset);
            *symbols_size = section_headers[i].sh_size;
        } else if (section_headers[i].sh_type == 3 && strcmp(".strtab", section_string_table + section_headers[i].sh_name) == 0) {
            *string_table = (char *) (((uintptr_t) buffer) + section_headers[i].sh_offset);
        }
    }
}

void init_kernel_symbols(void) {
    int ret = fs_read_all_path("/boot/os_2.o", &kernel_buffer, NULL, NULL);
    if (ret) {
        debug_log("failed to read kernel object file: [ %s ]\n", strerror(-ret));
        return;
    }

    if (!elf64_is_valid(kernel_buffer)) {
        debug_log("kernel object is file is not elf64?\n");
        free(kernel_buffer);
        kernel_buffer = NULL;
        return;
    }

    try_load_symbols(kernel_buffer, &kernel_symbols, &kernel_symbols_size, &kernel_string_table);
    if (!kernel_symbols || !kernel_string_table) {
        debug_log("failed to load kernel symbols\n");
        free(kernel_buffer);
        kernel_buffer = NULL;
    } else {
        debug_log("successfully loaded kernel symbols\n");
    }
}

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

void elf64_load_program(void *buffer, size_t length, struct file *execuatable, struct task *task) {
    Elf64_Ehdr *elf_header = buffer;
    Elf64_Phdr *program_headers = (Elf64_Phdr *) (((uintptr_t) buffer) + elf_header->e_phoff);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        uintptr_t program_section_start = program_headers[i].p_vaddr;
        if (program_section_start == 0) {
            continue;
        }

        if (program_headers[i].p_type == 7) {
            program_section_start = (elf64_get_start(buffer) - program_headers[i].p_memsz) & ~0xFFFULL;
            task->process->tls_master_copy_start = (void *) program_section_start;
            task->process->tls_master_copy_size = program_headers[i].p_memsz;
            task->process->tls_master_copy_alignment = program_headers[i].p_align;
        }

        assert(program_section_start < ((uintptr_t) buffer) + length);

        uintptr_t program_section_end = program_section_start + program_headers[i].p_memsz;
        uintptr_t aligned_start = program_section_start & ~0xFFFULL;
        uintptr_t aligned_end = ((program_section_end + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
        int protections = program_headers[i].p_flags == 0x5 ? (PROT_READ | PROT_EXEC) : (PROT_READ | PROT_WRITE);
        uint64_t type =
            program_headers[i].p_type == 7 ? VM_PROCESS_TLS_MASTER_COPY : protections & PROT_EXEC ? VM_PROCESS_TEXT : VM_PROCESS_DATA;

        // This will be true for the text segment
        if (program_headers[i].p_memsz == program_headers[i].p_filesz && program_section_start % PAGE_SIZE == 0) {
            assert(fs_mmap((void *) aligned_start, program_headers[i].p_filesz, PROT_READ | PROT_EXEC, MAP_SHARED, execuatable,
                           program_headers[i].p_offset) != (intptr_t) MAP_FAILED);
        } else {
            struct vm_region *added = map_region((void *) aligned_start, aligned_end - aligned_start, PROT_WRITE, type);

            struct vm_object *object = vm_create_anon_object(aligned_end - aligned_start);
            added->vm_object = object;
            added->vm_object_offset = 0;

            vm_map_region_with_object(added);

#ifdef ELF64_DEBUG
            debug_log("program section: [ %#.16lX, %#.16lX, %#.16lX, %#.16lX, %#.16lX ]\n", program_section_start,
                      program_headers[i].p_offset, program_headers[i].p_filesz, program_headers[i].p_memsz, program_section_end);
#endif /* ELF64_DEBUG */

            // FIXME: check for the possibility of a read only section here, we would need to temporarily change protections
            //        in that case.
            memcpy((char *) program_section_start, ((char *) buffer) + program_headers[i].p_offset, program_headers[i].p_filesz);
        }
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
    task_heap->vm_object = vm_create_anon_object(0);
    task_heap->vm_object_offset = 0;
    task->process->process_memory = add_vm_region(task->process->process_memory, task_heap);
}

struct stack_frame {
    struct stack_frame *next;
    uintptr_t rip;
};

static size_t print_symbol_at(uintptr_t rip, Elf64_Sym *symbols, uintptr_t symbols_size, const char *string_table,
                              size_t (*output)(void *closure, const char *string, ...), void *closure1, bool kernel) {
    for (int i = 0; symbols && string_table && (uintptr_t)(symbols + i) < ((uintptr_t) symbols) + symbols_size; i++) {
        if (symbols[i].st_name != 0 && symbols[i].st_size != 0) {
            if (rip >= symbols[i].st_value && rip <= symbols[i].st_value + symbols[i].st_size) {
                return output(closure1, "<%s> [ %#.16lX, %#.16lX, %s ]\n", kernel ? "K" : "U", rip, symbols[i].st_value,
                              string_table + symbols[i].st_name);
            }
        }
    }

    // We didn't find a matching symbol
    return output(closure1, "<%s> [ %#.16lX, %#.16lX, %s ]\n", kernel ? "K" : "U", rip, 0UL, "??");
}

static size_t do_stack_trace(uintptr_t rip, uintptr_t rbp, Elf64_Sym *symbols, size_t symbols_size, char *string_table,
                             size_t (*output)(void *closure, const char *string, ...), void *closure1,
                             uintptr_t (*mapper)(void *closure, uintptr_t address), void *closure2, bool kernel) {
    size_t ret = print_symbol_at(rip, symbols, symbols_size, string_table, output, closure1, kernel);

    struct stack_frame *frame = (struct stack_frame *) (mapper ? mapper(closure2, rbp) : rbp);
    int (*validate)(const void *addr, size_t size) = mapper ? NULL : kernel ? &validate_kernel_read : &validate_read;
    while ((!validate || !validate(frame, sizeof(struct stack_frame))) && frame) {
        struct stack_frame *old_frame = frame;
        frame = frame->next;
        frame = (struct stack_frame *) (mapper ? mapper(closure2, (uintptr_t) frame) : (uintptr_t) frame);
        if (!old_frame->rip) {
            break;
        }
        ret += print_symbol_at(old_frame->rip, symbols, symbols_size, string_table, output, closure1, kernel);
    }

    return ret;
}

// NOTE: this must be called from within a task's address space
size_t do_elf64_stack_trace(struct task *task, bool extra_info, size_t (*output)(void *closure, const char *string, ...), void *closure) {
    if (extra_info) {
        dump_process_regions(task->process);
    }

    struct inode *inode = task->process->exe->inode;

    assert(inode->i_op->mmap);
    void *buffer = (void *) inode->i_op->mmap(NULL, inode->size, PROT_READ, MAP_SHARED, inode, 0);
    if (buffer == MAP_FAILED) {
        debug_log("Failed to read the task's inode: [ %d ]\n", task->process->pid);
        return 0;
    }

    if (!elf64_is_valid(buffer)) {
        debug_log("The task is not elf64: [ %d ]\n", task->process->pid);
        return 0;
    }

    Elf64_Sym *symbols = NULL;
    char *string_table = NULL;
    size_t symbols_size = 0;

    try_load_symbols(buffer, &symbols, &symbols_size, &string_table);

    uintptr_t rsp = task->in_kernel ? task->arch_task.user_task_state->stack_state.rsp : task->arch_task.task_state.stack_state.rsp;
    uintptr_t rbp = task->in_kernel ? task->arch_task.user_task_state->cpu_state.rbp : task->arch_task.task_state.cpu_state.rbp;
    uintptr_t rip = task->in_kernel ? task->arch_task.user_task_state->stack_state.rip : task->arch_task.task_state.stack_state.rip;

    if (!symbols || !string_table) {
        debug_log("No symbols or string table (probably stripped binary)\n");
    }

    if (extra_info) {
        debug_log("Dumping core: [ %#.16lX, %#.16lX ]\n", rip, rsp);
    }

    size_t ret = do_stack_trace(rip, rbp, symbols, symbols_size, string_table, output, closure, NULL, NULL, false);

    unmap_range((uintptr_t) buffer, ((inode->size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE);

    return ret;
}

static size_t debug_log_wrapper(void *closure __attribute__((unused)), const char *string, ...) {
    va_list parameters;
    va_start(parameters, string);

    size_t ret = vdebug_log(string, parameters);

    va_end(parameters);
    return ret;
}

void elf64_stack_trace(struct task *task, bool extra_info) {
    do_elf64_stack_trace(task, extra_info, debug_log_wrapper, NULL);
}

void kernel_stack_trace(uintptr_t instruction_pointer, uintptr_t frame_base) {
    do_stack_trace(instruction_pointer, frame_base, kernel_symbols, kernel_symbols_size, kernel_string_table, debug_log_wrapper, NULL, NULL,
                   NULL, true);

    struct task *current = get_current_task();
    if (current->in_kernel && !current->kernel_task) {
        elf64_stack_trace(current, false);
    }
}

struct snprintf_object {
    char *buffer;
    size_t max;
};

static size_t snprintf_wrapper(void *closure, const char *string, ...) {
    va_list parameters;
    va_start(parameters, string);

    struct snprintf_object *cls = closure;
    size_t ret = vsnprintf(cls->buffer, cls->max, string, parameters);
    cls->max = cls->max - ret > cls->max ? 0 : cls->max - ret;
    cls->buffer += ret;

    va_end(parameters);
    return ret;
}

static uintptr_t kernel_stack_mapper(void *closure, uintptr_t address) {
    uintptr_t virt_base = (uintptr_t) closure;
    uintptr_t mapped = address - KERNEL_TASK_STACK_START + virt_base + PAGE_SIZE;

    if (mapped - virt_base > PAGE_SIZE) {
        return 0;
    }
    return mapped;
}

size_t kernel_stack_trace_for_procfs(struct task *main_task, void *buffer, size_t buffer_max) {
    // Can't stack trace the idle task
    if (main_task->tid == 1) {
        return 0;
    }

    struct snprintf_object obj = { .buffer = buffer, .max = buffer_max };
    if (main_task->kernel_task || main_task->in_kernel) {
        uintptr_t stack_phys_mapping =
            (uintptr_t) create_phys_addr_mapping(main_task->arch_task.kernel_stack_info->pt_entry & (~0xFFF ^ (1ULL << 63ULL)));

        struct task *current = get_current_task();

        uintptr_t rip =
            current == main_task ? (uintptr_t)(&kernel_stack_trace_for_procfs) : main_task->arch_task.task_state.stack_state.rip;

        uintptr_t current_rbp;
        asm("mov %%rbp, %0" : "=r"(current_rbp) : :);
        uintptr_t rbp = main_task ? current_rbp : main_task->arch_task.task_state.cpu_state.rbp;
        return do_stack_trace(rip, rbp, kernel_symbols, kernel_symbols_size, kernel_string_table, snprintf_wrapper, &obj,
                              kernel_stack_mapper, (void *) stack_phys_mapping, true);
    }

    return 0;
}
