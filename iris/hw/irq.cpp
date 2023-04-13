#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/hw/irq.h>
#include <iris/hw/irq_controller.h>
#include <iris/hw/power.h>
#include <iris/uapi/syscall.h>

namespace iris {
Expected<void> register_external_irq_handler(IrqLine line, IrqHandler handler) {
    auto irq = TRY(irq_number_for_legacy_isa_interrupt_number(line));

    auto& irq_handlers = global_state().irq_handlers;
    return irq_handlers.with_lock([&](auto& irq_handlers) -> Expected<void> {
        TRY(irq_handlers[irq.raw_value()].emplace_back(di::move(handler)) & [](auto&&) {
            return Error::NotEnoughMemory;
        });

        auto& irq_controller = TRY(irq_controller_for_interrupt_number(irq));
        irq_controller.with_lock([&](auto& controller) {
            enable_irq_line(controller, line);
        });
        return {};
    });
}

Expected<void> register_exception_handler(GlobalIrqNumber irq, IrqHandler handler) {
    auto& irq_handlers = global_state().irq_handlers;
    return irq_handlers.with_lock([&](auto& irq_handlers) -> Expected<void> {
        TRY(irq_handlers[irq.raw_value()].emplace_back(di::move(handler)) & [](auto&&) {
            return Error::NotEnoughMemory;
        });
        return {};
    });
}

extern "C" void generic_irq_handler(GlobalIrqNumber irq, iris::arch::TaskState& task_state, int error_code) {
    if (!task_state.in_kernel()) {
        setup_current_processor_access();
    }

    // SAFETY: this is safe since interrupts are disabled.
    auto& current_task = current_processor_unsafe().scheduler().current_task();
    auto guard = di::ScopeExit([&] {
        if (!task_state.in_kernel()) {
            arch::load_userspace_thread_pointer(current_task.userspace_thread_pointer(), task_state);
        }
    });

    auto context = IrqContext { task_state, error_code, irq_controller_for_interrupt_number(irq).optional_value() };

    // Syscall IRQ vector.
    if (irq == GlobalIrqNumber(0x80)) {
        raw_enable_interrupts();
        auto result = do_syscall(current_task, task_state);
        task_state.set_syscall_return(result);
        return;
    }

    {
        auto handlers = global_state().irq_handlers.lock();
        for (auto& handler : (*handlers)[irq.raw_value()]) {
            if (handler(context) == IrqStatus::Handled) {
                return;
            }
        }
    }

    iris::println("ERROR: got unexpected IRQ {}, error_code={}, ip={:#x}"_sv, irq, error_code, task_state.rip);
    ASSERT(false);
}
}
