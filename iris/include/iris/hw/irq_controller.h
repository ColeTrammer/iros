#pragma once

#include <di/prelude.h>

namespace iris {
struct GlobalIrqNumberTag {
    using Type = u8;
};

using GlobalIrqNumber = di::StrongInt<GlobalIrqNumberTag>;

struct IrqLineTag {
    using Type = u8;
};

using IrqLine = di::StrongInt<IrqLineTag>;

namespace detail {
    struct SendEoiFunction : di::Dispatcher<SendEoiFunction, void(di::This&, GlobalIrqNumber)> {};
    struct EnableIrqLine : di::Dispatcher<EnableIrqLine, void(di::This&, GlobalIrqNumber)> {};
    struct DisableIrqLine : di::Dispatcher<DisableIrqLine, void(di::This&, GlobalIrqNumber)> {};
}

constexpr inline auto send_eoi = detail::SendEoiFunction {};
constexpr inline auto enable_irq_line = detail::EnableIrqLine {};
constexpr inline auto disable_irq_line = detail::DisableIrqLine {};

using IrqControllerInterface = di::meta::List<detail::SendEoiFunction, detail::EnableIrqLine, detail::DisableIrqLine>;
using IrqController = di::AnyInline<IrqControllerInterface>;

di::Optional<di::Synchronized<IrqController>&> irq_controller_for_interrupt_number(GlobalIrqNumber number);
}
