#ifndef _KERNEL_PROC_ELF64_H
#define _KERNEL_PROC_ELF64_H 1

#include <elf.h>
#include <stdbool.h>

#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>

struct file;

bool elf64_is_valid(void *buffer);
uintptr_t elf64_get_start(void *buffer);

uintptr_t elf64_load_program(void *buffer, size_t length, struct file *executable, struct initial_process_info *info);
void elf64_map_heap(struct task *task, struct initial_process_info *info);
struct vm_region *elf64_create_vm_region(void *buffer, uint64_t type);
void elf64_stack_trace(struct task *task, bool extra_info);

size_t kernel_stack_trace_for_procfs(struct task *main_task, void *buffer, size_t buffer_max);
void kernel_stack_trace(uintptr_t instruction_pointer, uintptr_t frame_base);
void init_kernel_symbols(void);

#endif /* _KERNEL_PROC_ELF64_H */
