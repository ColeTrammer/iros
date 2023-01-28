#pragma once

#include <di/meta/bool_constant.h>
#include <di/meta/list/invoke.h>

namespace di::meta {
template<concepts::MetaInvocable MetaFn>
struct Not {
    template<typename... Args>
    using Invoke = meta::BoolConstant<!meta::Invoke<MetaFn, Args...>::value>;
};
}