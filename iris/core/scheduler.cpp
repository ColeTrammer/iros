#include <iris/core/print.h>
#include <iris/core/scheduler.h>

namespace iris {
void Scheduler::schedule_task(Task& task) {
    m_run_queue.push(task);
}

static void do_idle() {
    // x86_64 specific. Enables IRQs and then halt.

    for (;;) {
        asm volatile("sti\n"
                     "hlt\n"
                     "cli\n");
    }
}

void Scheduler::start() {
    // Initialize the idle task.
    m_idle_task = *create_kernel_task(do_idle);

    run_next();
}

[[gnu::naked]] void Scheduler::yield() {
    // To yield a task, we must first save its current state, so that
    // it can be resumed later. Instead of setting %rip based on the
    // current instruction pointer, set %rip to a later address which
    // is then the point at which the current task is resumed. Once the
    // task state is saved, simply call Scheduler::save_state_and_run_next()
    // which performs the context switch to the next task. In the current
    // implementation, the task state is pushed onto the stack, which is then
    // passed to a c++ function. It would be more efficent to save the relevant
    // registers directly into the current task pointer, but this would be even
    // more complicated. Additionally, only registers which are not preserved by
    // the SYS-V ABI need to be saved.
    asm volatile("movabsq $_continue, %rax\n"
                 "movq %rsp, %rdx\n"

                 "pushq $0x00\n"
                 "pushq %rdx\n"
                 "pushfq\n"
                 "pushq $0x28\n"
                 "pushq %rax\n"

                 "push %rax\n"
                 "push %rbx\n"
                 "push %rcx\n"
                 "push %rdx\n"
                 "push %rsi\n"
                 "push %rdi\n"
                 "push %rbp\n"
                 "push %r8\n"
                 "push %r9\n"
                 "push %r10\n"
                 "push %r11\n"
                 "push %r12\n"
                 "push %r13\n"
                 "push %r14\n"
                 "push %r15\n"

                 "mov %rsp, %rsi\n"
                 "jmp _ZN4iris9Scheduler23save_state_and_run_nextEPNS_4arch9TaskStateE\n"

                 "_continue:\n"
                 "ret\n");
}

void Scheduler::run_next() {
    // If there is nothing to run, execute the idle task.
    auto& next = m_run_queue.pop().value_or(*m_idle_task);
    m_current_task = di::addressof(next);
    next.context_switch_to();
}

void Scheduler::save_state_and_run_next(arch::TaskState* task_state) {
    asm volatile("cli");

    // Ensure that we never put the idle task into the run queue.
    if (m_current_task != m_idle_task.get()) {
        m_run_queue.push(*m_current_task);
    }

    m_current_task->set_task_state(*task_state);
    run_next();
}

void Scheduler::exit_current_task() {
    {
        // Construct a temporary Arc to the task. By using retain_object, the reference
        // count is not incremented. Thus, when task_reference goes out of scope, the task's
        // reference count will decrement, likely freeing its memory. This reference count was
        // reserved in the Task's constructor, to only be decremented once exit_task() is
        // explicitly called.
        auto task_reference = di::Arc<Task>(m_current_task, di::retain_object);
    }

    asm volatile("cli");
    // NOTE: by not pushing the current task into the run queue, it will
    //       not get scheduled again.
    run_next();
}
}
