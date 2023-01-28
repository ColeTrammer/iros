#pragma once

#include <liim/construct.h>
#include <liim/container/concepts.h>
#include <liim/container/consumer/collect.h>
#include <liim/result.h>

namespace LIIM {
template<typename T, LIIM::Container::Container C>
constexpr decltype(auto) assign_to(T& output, C&& container) requires(!AssignableFrom<T, C>) {
    return result_and_then(::collect<T>(forward<C>(container)), [&](auto&& result) -> T& {
        return output = forward<decltype(result)&&>(result);
    });
}
}

using LIIM::assign_to;
