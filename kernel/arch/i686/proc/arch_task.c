#include <kernel/hal/hal.h>
#include <kernel/proc/task.h>

static void kernel_idle() {
    for (;;) {
        asm volatile("sti\n"
                     "hlt\n"
                     "cli\n"
                     :
                     :
                     : "memory");
        kernel_yield();
    }
}

static void load_task_into_memory(struct task *task) {
    if (task->kernel_stack) {
        set_tss_stack_pointer(task->kernel_stack->end);
    }

    if (task->process->arch_process.cr3 != get_cr3()) {
        load_cr3(task->process->arch_process.cr3);
    }

    if (!task->kernel_task) {
        fxrstor(task->fpu.aligned_state);
        // set_msr(MSR_FS_BASE, (uint64_t) task->arch_task.user_thread_pointer);
    }
}

/* Must be called from unpremptable context */
void arch_run_task(struct task *task) {
    load_task_into_memory(task);
    set_current_task(task);
    __run_task(&task->arch_task);
}

void task_setup_user_state(struct task_state *task_state) {
    task_state->stack_state.cs = USER_CODE_SELECTOR;
    task_state->stack_state.eflags = get_rflags() | INTERRUPTS_ENABLED_FLAG;
    task_state->stack_state.ss = USER_DATA_SELECTOR;
}

void arch_load_kernel_task(struct task *task, uintptr_t entry) {
    task->process->arch_process.cr3 = idle_kernel_process.arch_process.cr3;
    task->arch_task.task_state.cpu_state.ebp = task->kernel_stack->end;
    task->arch_task.task_state.stack_state.eip = entry;
    task->arch_task.task_state.stack_state.cs = CS_SELECTOR;
    task->arch_task.task_state.stack_state.eflags = get_rflags() | INTERRUPTS_ENABLED_FLAG;
    task->arch_task.task_state.stack_state.esp = task->kernel_stack->end;
    task->arch_task.task_state.stack_state.ss = DATA_SELECTOR;
    task->in_kernel = true;
}

void arch_init_idle_task(struct task *idle_task, struct processor *processor) {
    idle_task->arch_task.task_state.stack_state.eip = (uintptr_t) &kernel_idle;
    idle_task->arch_task.task_state.stack_state.cs = CS_SELECTOR;
    idle_task->arch_task.task_state.stack_state.eflags = get_rflags() & ~INTERRUPTS_ENABLED_FLAG;
    idle_task->arch_task.task_state.stack_state.ss = DATA_SELECTOR;
    idle_task->arch_task.task_state.stack_state.esp = processor->kernel_stack->end;

    debug_log("idle task: [ %d ]\n", idle_task->tid);

    task_align_fpu(idle_task);
    fninit();
    fxsave(idle_task->fpu.aligned_state);
}

void arch_setup_program_args(struct task *task, struct initial_process_info *info, size_t argc, char **argv, char **envp) {
    task->user_task_state->cpu_state.eax = (uint32_t) info;
    task->user_task_state->cpu_state.edi = (uint32_t) argc;
    task->user_task_state->cpu_state.esi = (uint32_t) argv;
    task->user_task_state->cpu_state.ecx = (uint32_t) envp;
}

void task_do_sig_handler(struct task *task, int signum) {
    (void) task;
    (void) signum;
    assert(false);
}

/* Must be called from unpremptable context */
void arch_free_task(struct task *task, bool free_paging_structure) {
    (void) task;
    (void) free_paging_structure;
}
