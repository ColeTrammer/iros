#pragma once

#include <di/concepts/conjunction.h>
#include <di/concepts/same_as.h>
#include <di/meta/bool_constant.h>

namespace di::meta {
template<typename T>
struct SameAs {
    template<typename... Args>
    using Invoke = meta::BoolConstant<concepts::Conjunction<concepts::SameAs<T, Args>...>>;
};
}
