#pragma once

#include <iris/core/config.h>

// clang-format off
#include IRIS_ARCH_INCLUDE(core/interrupt_disabler.h)
// clang-format on

namespace iris {
template<di::concepts::Invocable F>
decltype(auto) with_interrupts_disabled(F&& function) {
    InterruptDisabler guard {};
    return di::invoke(di::forward<F>(function));
}
}
