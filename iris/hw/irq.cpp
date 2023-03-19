#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/hw/irq.h>
#include <iris/hw/irq_controller.h>
#include <iris/hw/power.h>
#include <iris/uapi/syscall.h>

namespace iris {
Expected<GlobalIrqNumber> irq_number_for_legacy_isa_interrupt_number(IrqLine irq_line) {
    return GlobalIrqNumber(irq_line.raw_value() + 32);
}

Expected<void> register_irq_handler(GlobalIrqNumber irq, IrqHandler handler) {
    auto& irq_handlers = global_state().irq_handlers;
    return irq_handlers.with_lock([&](auto& irq_handlers) -> Expected<void> {
        // FIXME: propogate allocation failures.
        irq_handlers[irq.raw_value()].push_back(di::move(handler));

        if (auto irq_controller = irq_controller_for_interrupt_number(irq)) {
            irq_controller->with_lock([&](auto& controller) {
                enable_irq_line(controller, irq);
            });
        }
        return {};
    });
}

extern "C" void generic_irq_handler(GlobalIrqNumber irq, iris::arch::TaskState& task_state, int error_code) {
    auto context = IrqContext { task_state, error_code, irq_controller_for_interrupt_number(irq) };

    // Syscall IRQ vector.
    if (irq == GlobalIrqNumber(0x80)) {
        auto result = do_syscall(global_state().scheduler.current_task(), task_state);
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
