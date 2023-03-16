#include <iris/hw/irq_controller.h>

namespace iris {
di::Optional<IrqController&> irq_controller_for_interrupt_number(GlobalIrqNumber) {
    return di::nullopt;
}
}
