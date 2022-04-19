#include <kernel/hal/hal.h>
#include <kernel/proc/task.h>

#define SIZEOF_IRETQ_INSTRUCTION 2 // bytes

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
        arch_task_load_thread_self_pointer(task->arch_task.user_thread_pointer);
    }
}

void arch_sys_create_task(struct task *task, uintptr_t entry, uintptr_t new_sp, uintptr_t return_address, void *arg) {
    task_setup_user_state(&task->arch_task.task_state);
    task->arch_task.task_state.stack_state.eip = entry;

    // Stack must be 16 byte aligned as per the SYS-V ABI, and room must be
    // made for the function argument.
    new_sp -= 4;
    new_sp &= ~0xF;
    assert(new_sp % 16 == 0);

    *((uintptr_t *) new_sp) = (uintptr_t) arg;
    new_sp -= 4;
    *((uintptr_t *) new_sp) = return_address;

    task->arch_task.task_state.stack_state.esp = new_sp;
}

/* Must be called from unpremptable context */
void arch_run_task(struct task *task) {
    load_task_into_memory(task);
    set_current_task(task);
    __run_task(&task->arch_task);
}

void arch_task_switch_from_kernel_to_user_mode(struct task *task) {
    task_align_fpu(task);
}

void arch_task_set_thread_self_pointer(struct task *task, void *thread_self_pointer) {
    task->arch_task.user_thread_pointer = thread_self_pointer;
    arch_task_load_thread_self_pointer(thread_self_pointer);
}

void arch_task_prepare_to_restart_sys_call(struct task *task) {
    // The resume point of the task will be its rip minus the size of the system call
    // entry instruction. The eax register is also reset to the system call number, so
    // that system call will be properly resumed.
    task->user_task_state->stack_state.eip -= SIZEOF_IRETQ_INSTRUCTION;
    task->user_task_state->cpu_state.eax = task->last_system_call;
}

void task_setup_user_state(struct task_state *task_state) {
    task_state->cpu_state.ds = USER_DATA_SELECTOR;
    task_state->cpu_state.es = USER_DATA_SELECTOR;
    task_state->cpu_state.fs = USER_DATA_SELECTOR;
    task_state->cpu_state.gs = GS_USER_THREAD_SELECTOR;

    task_state->stack_state.cs = USER_CODE_SELECTOR;
    task_state->stack_state.eflags = INTERRUPTS_ENABLED_FLAG;
    task_state->stack_state.ss = USER_DATA_SELECTOR;
}

void arch_load_kernel_task(struct task *task, uintptr_t entry) {
    task->process->arch_process.cr3 = idle_kernel_process.arch_process.cr3;
    task->arch_task.task_state.cpu_state.ebp = task->kernel_stack->end;

    task->arch_task.task_state.cpu_state.ds = DATA_SELECTOR;
    task->arch_task.task_state.cpu_state.es = DATA_SELECTOR;
    task->arch_task.task_state.cpu_state.fs = DATA_SELECTOR;
    task->arch_task.task_state.cpu_state.gs = GS_PROCESSOR_SELECTOR;

    task->arch_task.task_state.stack_state.eip = entry;
    task->arch_task.task_state.stack_state.cs = CS_SELECTOR;
    task->arch_task.task_state.stack_state.eflags = get_rflags() | INTERRUPTS_ENABLED_FLAG;
    task->arch_task.task_state.stack_state.esp = task->kernel_stack->end;
    task->arch_task.task_state.stack_state.ss = DATA_SELECTOR;
    task->in_kernel = true;
}

void arch_init_idle_task(struct task *idle_task, struct processor *processor) {
    idle_task->arch_task.task_state.cpu_state.ds = DATA_SELECTOR;
    idle_task->arch_task.task_state.cpu_state.es = DATA_SELECTOR;
    idle_task->arch_task.task_state.cpu_state.fs = DATA_SELECTOR;
    idle_task->arch_task.task_state.cpu_state.gs = GS_PROCESSOR_SELECTOR;

    idle_task->arch_task.task_state.stack_state.eip = (uintptr_t) &kernel_idle;
    idle_task->arch_task.task_state.stack_state.cs = CS_SELECTOR;
    idle_task->arch_task.task_state.stack_state.eflags = get_rflags() & ~INTERRUPTS_ENABLED_FLAG;
    idle_task->arch_task.task_state.stack_state.ss = DATA_SELECTOR;
    idle_task->arch_task.task_state.stack_state.esp = processor->kernel_stack->end;

    task_align_fpu(idle_task);
    fninit();
    fxsave(idle_task->fpu.aligned_state);
}

char *arch_setup_program_args(struct task *, char *args_start, struct initial_process_info *info, size_t argc, char **argv, char **envp) {
    // Pass arguments on the stack, and ensure things are 16 byte aligned, with an additional 0 word representing the
    // return address, as per the SYS-V ABI.
    uint32_t *esp = (void *) ((uintptr_t) args_start & ~0xF);
    *--esp = (uint32_t) envp;
    *--esp = (uint32_t) argv;
    *--esp = (uint32_t) argc;
    *--esp = (uint32_t) info;

    assert((uintptr_t) esp % 16 == 0);
    *--esp = 0;

    return (void *) esp;
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
