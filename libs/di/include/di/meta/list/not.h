#pragma once

#include <di/meta/constexpr.h>
#include <di/meta/list/invoke.h>

namespace di::meta {
template<concepts::MetaInvocable MetaFn>
struct Not {
    template<typename... Args>
    using Invoke = Constexpr<!meta::Invoke<MetaFn, Args...>::value>;
};
}
