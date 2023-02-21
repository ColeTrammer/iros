#pragma once

#include <iris/core/config.h>

// clang-format off
#include IRIS_ARCH_INCLUDE(core/userspace_access.h)
// clang-format on

namespace iris {

template<di::concepts::Invocable F>
decltype(auto) with_userspace_access(F&& function) {
    UserspaceAccessEnabler guard {};
    return di::invoke(di::forward<F>(function));
}
}
