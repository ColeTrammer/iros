#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/constexpr.h>

namespace di::meta {
template<typename T>
struct SameAs {
    template<typename... Args>
    using Invoke = Constexpr<(concepts::SameAs<T, Args> && ...)>;
};
}
