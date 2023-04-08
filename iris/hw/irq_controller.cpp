#include <iris/core/global_state.h>
#include <iris/hw/irq_controller.h>

namespace iris {
Expected<di::Synchronized<IrqController>&> irq_controller_for_interrupt_number(GlobalIrqNumber irq_number) {
    if (irq_number >= GlobalIrqNumber(32) && irq_number < GlobalIrqNumber(32 + 16)) {
        return global_state().irq_controller;
    }
    return di::Unexpected(Error::ArgumentOutOfDomain);
}
}
