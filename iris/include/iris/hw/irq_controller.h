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

struct IrqLineRange {
    IrqLine start;
    IrqLine end_inclusive;
};

namespace detail {
    struct SendEoiFunction : di::Dispatcher<SendEoiFunction, void(di::This&, IrqLine)> {};
    struct EnableIrqLine : di::Dispatcher<EnableIrqLine, void(di::This&, IrqLine)> {};
    struct DisableIrqLine : di::Dispatcher<DisableIrqLine, void(di::This&, IrqLine)> {};
    struct ResponsibleIrqLineRange : di::Dispatcher<ResponsibleIrqLineRange, IrqLineRange(di::This&)> {};
}

constexpr inline auto send_eoi = detail::SendEoiFunction {};
constexpr inline auto enable_irq_line = detail::EnableIrqLine {};
constexpr inline auto disable_irq_line = detail::DisableIrqLine {};
constexpr inline auto responsible_irq_line_range = detail::ResponsibleIrqLineRange {};

using IrqControllerInterface = di::meta::List<detail::SendEoiFunction, detail::EnableIrqLine, detail::DisableIrqLine,
                                              detail::ResponsibleIrqLineRange>;
using IrqController = di::AnyInline<IrqControllerInterface>;

Expected<di::Synchronized<IrqController>&> irq_controller_for_interrupt_number(GlobalIrqNumber number);
}
