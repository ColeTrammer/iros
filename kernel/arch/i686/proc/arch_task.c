#include <kernel/proc/task.h>

void arch_setup_program_args(struct task *task, struct initial_process_info *info, size_t argc, char **argv, char **envp) {
    task->user_task_state->cpu_state.eax = (uint32_t) info;
    task->user_task_state->cpu_state.edi = (uint32_t) argc;
    task->user_task_state->cpu_state.esi = (uint32_t) argv;
    task->user_task_state->cpu_state.ecx = (uint32_t) envp;
}
