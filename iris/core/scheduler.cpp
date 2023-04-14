#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>
#include <iris/core/print.h>
#include <iris/core/scheduler.h>
#include <iris/hw/irq.h>

namespace iris {
void Scheduler::schedule_task(Task& task) {
    task.set_runnable();
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
    setup_idle_task();

    // Setup timer interrupt.
    *register_external_irq_handler(IrqLine(0), [](IrqContext& context) -> IrqStatus {
        send_eoi(*context.controller->lock(), IrqLine(0));

        // SAFETY: This is safe since interrupts are disabled.
        auto& scheduler = current_processor_unsafe().scheduler();

        // If preemption is disabled, do not reshcedule the currently running task but let it know
        // that it should yield whenever it finally re-enables preemption.
        auto& current_task = scheduler.current_task();
        if (current_task.preemption_disabled()) {
            current_task.set_should_be_preempted();
            return IrqStatus::Handled;
        }

        // Manually unlock the IRQ list before jumping away.
        global_state().irq_handlers.get_lock().unlock();
        scheduler.save_state_and_run_next(&context.task_state);
        return IrqStatus::Handled;
    });

    // SAFETY: This is safe since interrupts are disabled.
    current_processor_unsafe().mark_as_online();
    run_next();
}

void Scheduler::setup_idle_task() {
    m_idle_task = *create_kernel_task(global_state().task_namespace, do_idle);
}

void Scheduler::start_on_ap() {
    // SAFETY: This is safe since interrupts are disabled.
    current_processor_unsafe().mark_as_online();
    run_next();
}

[[gnu::naked]] void Scheduler::yield() {
    // To yield a task, we must first save its current state, so that it can be resumed later. Instead of setting %rip
    // based on the current instruction pointer, set %rip to a later address which is then the point at which the
    // current task is resumed. Once the task state is saved, simply call Scheduler::save_state_and_run_next() which
    // performs the context switch to the next task. In the current implementation, the task state is pushed onto the
    // stack, which is then passed to a c++ function. It would be more efficent to save the relevant registers directly
    // into the current task pointer, but this would be even more complicated. Additionally, only registers which are
    // not preserved by the SYS-V ABI need to be saved. Also, when the task is resumed, interrupts will be enabled.
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
    raw_disable_interrupts();

    // Ensure that we never put the idle task into the run queue, or waiting tasks.
    if (m_current_task != m_idle_task.get() && !m_current_task->waiting()) {
        m_run_queue.push(*m_current_task);
    }

    m_current_task->set_task_state(*task_state);

    // If this task has FPU state, save it.
    m_current_task->fpu_state().save();

    run_next();
}

void Scheduler::exit_current_task() {
    // Unregister the task from its task namespace.
    m_current_task->task_namespace().lock()->unregister_task(*m_current_task);

    // Store the task status so it remains valid after deleting the current task reference.
    auto task_status = m_current_task->task_status();

    {
        // Construct a temporary Arc to the task. By using retain_object, the reference
        // count is not incremented. Thus, when task_reference goes out of scope, the task's
        // reference count will decrement, likely freeing its memory. This reference count was
        // reserved in the Task's constructor, to only be decremented once exit_task() is
        // explicitly called.
        auto task_reference = di::Arc<Task>(m_current_task, di::retain_object);
    }

    raw_disable_interrupts();

    // Notify waiters that the task has exited.
    task_status->set_exited();

    // NOTE: by not pushing the current task into the run queue, it will
    //       not get scheduled again.
    run_next();
}

mm::AddressSpace& Scheduler::current_address_space() {
    if (!m_current_task) {
        return global_state().kernel_address_space;
    }
    return current_task().address_space();
}

Expected<void> Scheduler::block_current_task(di::FunctionRef<void()> before_yielding) {
    auto disabler = InterruptDisabler {};

    m_current_task->set_waiting();

    before_yielding();

    // NOTE: in the future, yield may return an error if the task is interrupted from userspace.
    yield();
    return {};
}

void schedule_task(Task& task) {
    auto local_schedule = [&] {
        with_interrupts_disabled([&] {
            // SAFETY: interrupts are disabled.
            auto& current_processor = current_processor_unsafe();
            current_processor.scheduler().schedule_task(task);
        });
    };

    auto const& global_state = iris::global_state();
    if (!global_state.all_aps_booted.load(di::MemoryOrder::Relaxed)) {
        local_schedule();
        return;
    }

    auto current_processor = iris::current_processor();
    auto next_processor_id = global_state.next_processor_to_schedule_on.fetch_add(1, di::MemoryOrder::Relaxed);
    next_processor_id %= global_state.alernate_processors.size() + 1;

    if (current_processor->id() == next_processor_id) {
        local_schedule();
        return;
    }

    current_processor->send_ipi(next_processor_id, [&](IpiMessage& message) {
        message.task_to_schedule = &task;
    });
}
}
