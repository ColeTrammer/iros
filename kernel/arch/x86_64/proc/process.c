#include <kernel/mem/page.h>
#include <kernel/proc/process.h>

void proc_kill_arch_process(struct process *process, bool free_paging_structure) {
    if (free_paging_structure) {
        remove_paging_structure(process->arch_process.cr3, process->process_memory);
    }
}
