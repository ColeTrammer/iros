#pragma once

#include <liim/container/consumer/insert.h>
#include <liim/result.h>

namespace LIIM::Container::Consumer {
template<typename T, Container C>
constexpr auto collect(C&& container) {
    auto result = T();
    return result_and_then(insert(result, result.end(), forward<C>(container)), [&](auto) -> T {
        return move(result);
    });
}
}

using LIIM::Container::Consumer::collect;
