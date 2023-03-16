#pragma once

#include <iris/core/task.h>
#include <iris/hw/irq_controller.h>

namespace iris {
struct IrqContext {
    arch::TaskState& task_state;
    int error_code {};
    di::Optional<di::Synchronized<IrqController>&> controller;
};

enum class IrqStatus {
    Handled,
    Unknown,
};

using IrqHandler = di::Function<IrqStatus(IrqContext&)>;

/// @brief Generic entry point called by assembly when an interrupt occurs.
///
/// @param irq The global irq number that was triggered
/// @param task_state The saved task state from the process which was interrupted
/// @param error_code The CPU error code, 0 if not present
extern "C" void generic_irq_handler(GlobalIrqNumber irq, iris::arch::TaskState& task_state, int error_code);

Expected<GlobalIrqNumber> irq_number_for_legacy_isa_interrupt_number(IrqLine irq_line);
Expected<void> register_irq_handler(GlobalIrqNumber number, IrqHandler handler);
}
