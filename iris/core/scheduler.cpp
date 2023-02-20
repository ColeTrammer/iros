#include <iris/core/print.h>
#include <iris/core/scheduler.h>

namespace iris {
void Scheduler::schedule_task(Task& task) {
    m_run_queue.push(task);
}

void Scheduler::start() {
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
    ASSERT(!m_run_queue.empty());

    auto& next = *m_run_queue.pop();
    m_current_task = di::addressof(next);
    next.context_switch_to();
}

void Scheduler::save_state_and_run_next(arch::TaskState* task_state) {
    asm volatile("cli");
    m_run_queue.push(*m_current_task);
    m_current_task->set_task_state(*task_state);
    run_next();
}
}