#pragma once

#include <di/function/invoke.h>
#include <iris/core/config.h>

#include IRIS_ARCH_INCLUDE(core/interrupt_disabler.h)

namespace iris {
template<di::concepts::Invocable F>
decltype(auto) with_interrupts_disabled(F&& function) {
    InterruptDisabler guard {};
    return di::function::invoke(di::util::forward<F>(function));
}
}
