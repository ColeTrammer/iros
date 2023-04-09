#include <iris/core/global_state.h>
#include <iris/hw/irq_controller.h>

namespace iris {
Expected<di::Synchronized<IrqController>&> irq_controller_for_interrupt_number(GlobalIrqNumber irq_number) {
    if (irq_number < global_state().arch_readonly_state.external_irq_offset) {
        return di::Unexpected(Error::ArgumentOutOfDomain);
    }

    auto relative_irq = IrqLine(irq_number - global_state().arch_readonly_state.external_irq_offset);
    for (auto& irq_controller : global_state().irq_controllers) {
        // SAFETY: this is safe because each IRQ controller cannot change its responsible IRQ range once it is
        // initialized.
        auto const range = responsible_irq_line_range(irq_controller.get_const_assuming_no_concurrent_mutations());
        if (range.start <= relative_irq && relative_irq <= range.end_inclusive) {
            return irq_controller;
        }
    }

    return di::Unexpected(Error::ArgumentOutOfDomain);
}
}
