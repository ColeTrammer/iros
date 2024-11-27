#include <iris/hw/irq.h>

#include <iris/core/global_state.h>

namespace iris {
auto irq_number_for_legacy_isa_interrupt_number(IrqLine irq_line) -> Expected<GlobalIrqNumber> {
    return GlobalIrqNumber(irq_line.raw_value() + global_state().arch_readonly_state.external_irq_offset);
}
}
