#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86_64/gdt.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

#define STACK_TRACE_ON_ANY_SIGSEGV

#define SIZEOF_IRETQ_INSTRUCTION 2 // bytes

extern struct process initial_kernel_process;

/* Default Args And Envp Passed to First Program */
static char first_arg[50];
static char *test_argv[3] = { "start", first_arg, NULL };

static char *test_envp[2] = { "PATH=/bin:/usr/bin:/initrd", NULL };

static void kernel_idle() {
    for (;;)
        ;
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
        set_msr(MSR_FS_BASE, (uint64_t) task->arch_task.user_thread_pointer);
    }
}

void task_align_fpu(struct task *task) {
    uintptr_t unaligned_fpu = (uintptr_t) &task->fpu.raw_fpu_state;
    task->fpu.aligned_state = (uint8_t *) ((unaligned_fpu & ~0xFULL) + 16ULL);
    assert(((uintptr_t) task->fpu.aligned_state) % 16 == 0);
    assert((uintptr_t) task->fpu.aligned_state >= unaligned_fpu &&
           (uintptr_t) task->fpu.aligned_state <= (uintptr_t) task->fpu.raw_fpu_state.image);
}

void arch_init_idle_task(struct task *idle_task, struct processor *processor) {
    idle_task->arch_task.task_state.stack_state.rip = (uint64_t) &kernel_idle;
    idle_task->arch_task.task_state.stack_state.cs = CS_SELECTOR;
    idle_task->arch_task.task_state.stack_state.rflags = get_rflags() | INTERRUPTS_ENABLED_FLAG;
    idle_task->arch_task.task_state.stack_state.ss = DATA_SELECTOR;
    idle_task->arch_task.task_state.stack_state.rsp = processor->kernel_stack->end;

    task_align_fpu(idle_task);
    fninit();
    fxsave(idle_task->fpu.aligned_state);
}

void arch_load_kernel_task(struct task *task, uintptr_t entry) {
    task->process->arch_process.cr3 = initial_kernel_process.arch_process.cr3;
    task->arch_task.task_state.cpu_state.rbp = task->kernel_stack->end;
    task->arch_task.task_state.stack_state.rip = entry;
    task->arch_task.task_state.stack_state.cs = CS_SELECTOR;
    task->arch_task.task_state.stack_state.rflags = get_rflags() | INTERRUPTS_ENABLED_FLAG;
    task->arch_task.task_state.stack_state.rsp = task->kernel_stack->end;
    task->arch_task.task_state.stack_state.ss = DATA_SELECTOR;
    task->in_kernel = true;
}

void arch_load_task(struct task *task, uintptr_t entry) {
    task->process->arch_process.cr3 = get_cr3();
    task->arch_task.task_state.cpu_state.rbp = 0;
    task->arch_task.task_state.stack_state.rip = entry;
    task->arch_task.task_state.stack_state.cs = USER_CODE_SELECTOR;
    task->arch_task.task_state.stack_state.rflags = get_rflags() | INTERRUPTS_ENABLED_FLAG;

    strcpy(test_argv[1], kernel_use_graphics() ? "-g" : "-v");
    proc_clone_program_args(task->process, NULL, test_argv, test_envp);
    task->arch_task.task_state.stack_state.rsp =
        map_program_args(get_vm_region(task->process->process_memory, VM_TASK_STACK)->end, task->process->args_context);

    task->arch_task.task_state.stack_state.ss = USER_DATA_SELECTOR;

    task_align_fpu(task);
    memcpy(task->fpu.aligned_state, get_idle_task()->fpu.aligned_state, FPU_IMAGE_SIZE);
}

/* Must be called from unpremptable context */
void arch_run_task(struct task *task) {
    load_task_into_memory(task);
    set_current_task(task);
    __run_task(&task->arch_task);
}

/* Must be called from unpremptable context */
void arch_free_task(struct task *task, bool free_paging_structure) {
    (void) task;
    (void) free_paging_structure;
}

void task_interrupt_blocking(struct task *task, int ret) {
    assert(task->blocking);
    task->arch_task.task_state.cpu_state.rax = (uint64_t) ret;
    task->blocking = false;
    task->sched_state = RUNNING_UNINTERRUPTIBLE;
}

bool proc_in_kernel(struct task *task) {
    return task->in_kernel;
}

void task_do_sig_handler(struct task *task, int signum) {
    assert(task->process->sig_state[signum].sa_handler != SIG_IGN);
    assert(task->process->sig_state[signum].sa_handler != SIG_DFL);

    if (task->blocking) {
        // Defer running the signal handler until after the blocking code
        // has a chance to clean up.
        task_interrupt_blocking(task, -EINTR);
        return;
    }

    // For debugging purposes (bash will catch SIGSEGV and try to cleanup)
#ifdef STACK_TRACE_ON_ANY_SIGSEGV
    if (signum == SIGSEGV) {
        elf64_stack_trace(task, true);
    }
#endif /* STACK_TRACE_ON_ANY_SIGSEGV */

    struct sigaction *act = &task->process->sig_state[signum];

    load_task_into_memory(task);

    uint64_t save_rsp =
        proc_in_kernel(task) ? task->arch_task.user_task_state->stack_state.rsp : task->arch_task.task_state.stack_state.rsp;
    if ((act->sa_flags & SA_ONSTACK) && (task->process->alt_stack.ss_flags & __SS_ENABLED)) {
        if (!(save_rsp >= (uintptr_t) task->process->alt_stack.ss_sp &&
              save_rsp <= (uintptr_t) task->process->alt_stack.ss_sp + task->process->alt_stack.ss_size)) {
            save_rsp = (((uintptr_t) task->process->alt_stack.ss_sp + task->process->alt_stack.ss_size)) & ~0xF;
        }
    }

    assert(save_rsp != 0);
    uint8_t *fpu_save_state = (uint8_t *) ((save_rsp - 128) & ~0xF) - FPU_IMAGE_SIZE; // Sub 128 to enforce red-zone
    ucontext_t *save_state = ((ucontext_t *) fpu_save_state) - 1;
    siginfo_t *info = ((siginfo_t *) save_state) - 1;
    if (act->sa_flags & SA_SIGINFO) {
        if (task->queued_signals->info.si_signo == signum) {
            memcpy(info, &task->queued_signals->info, sizeof(siginfo_t));
            task_dequeue_signal(task);
        } else {
            // NOTE: this probably shouldn't happen, but for now means that the
            //       signal was sent by kill instead of sigqueue.
            //       We probably should allocate a struct in this case
            info->si_code = SI_USER;
            task_unset_sig_pending(task, signum);
        }
    } else {
        task_unset_sig_pending(task, signum);
    }

    struct task_state *to_copy = proc_in_kernel(task) ? task->arch_task.user_task_state : &task->arch_task.task_state;

    uint64_t *stack_frame = ((uint64_t *) info) - 2;
    *stack_frame = (uintptr_t) act->sa_restorer;
    assert((uintptr_t) stack_frame % 16 == 0);

    save_state->uc_link = save_state;
    save_state->uc_sigmask = (uint64_t)(task->in_sigsuspend ? task->saved_sig_mask : task->sig_mask);

    if ((act->sa_flags & SA_ONSTACK) && (task->process->alt_stack.ss_flags & __SS_ENABLED)) {
        save_state->uc_stack.ss_flags = SS_ONSTACK;
        save_state->uc_stack.ss_size = task->process->alt_stack.ss_size;
        save_state->uc_stack.ss_sp = task->process->alt_stack.ss_sp;
    } else {
        save_state->uc_stack.ss_flags = SS_DISABLE;
        save_state->uc_stack.ss_size = 0;
        save_state->uc_stack.ss_sp = 0;
    }

    memcpy(&save_state->uc_mcontext, to_copy, sizeof(struct task_state));
    memcpy(fpu_save_state, task->fpu.aligned_state, FPU_IMAGE_SIZE);
    if (proc_in_kernel(task)) {
        if (task->in_sigsuspend) {
            task->in_sigsuspend = false;
            task->in_kernel = false;
        } else if ((act->sa_flags & SA_RESTART) && (task->arch_task.user_task_state->cpu_state.rax == (uint64_t) -EINTR)) {
            // Decrement %rip by the sizeof of the iretq instruction so that
            // the program will automatically execute int $0x80, restarting
            // the sys call in the easy way possible
            save_state->uc_mcontext.__stack_state.rip -= SIZEOF_IRETQ_INSTRUCTION;
        }
    }

    task->arch_task.task_state.cpu_state.rdi = signum;
    task->arch_task.task_state.cpu_state.rsi = (uintptr_t) info;
    task->arch_task.task_state.cpu_state.rdx = (uintptr_t) save_state;

    task->arch_task.task_state.stack_state.rip =
        ((act->sa_flags & SA_SIGINFO) ? (uintptr_t) act->sa_sigaction : (uintptr_t) act->sa_handler);
    task->arch_task.task_state.stack_state.rsp = (uintptr_t) stack_frame;
    task->arch_task.task_state.stack_state.ss = USER_DATA_SELECTOR;
    task->arch_task.task_state.stack_state.cs = USER_CODE_SELECTOR;

    task->sig_mask = act->sa_mask;
    if (!(act->sa_flags & SA_NODEFER)) {
        task->sig_mask |= (1U << (signum - 1));
    }

    if (act->sa_flags & SA_RESETHAND) {
        act->sa_handler = SIG_DFL;
    }

    debug_log("Running pid: [ %p, %#.16lX, %d ]\n", save_state, task->arch_task.task_state.stack_state.rip, signum);

    struct task *current = get_current_task();
    assert(current == task);
    current->sched_state = RUNNING_INTERRUPTIBLE;
    __run_task(&current->arch_task);
}
