#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/move.h>

namespace di::container {
template<typename In, typename Val>
struct InValueResult {
    template<typename I, typename V>
    requires(concepts::ConvertibleTo<In const&, I> && concepts::ConvertibleTo<Val const&, V>)
    constexpr operator InValueResult<I, V>() const& {
        return { in, value };
    }

    template<typename I, typename V>
    requires(concepts::ConvertibleTo<In, I> && concepts::ConvertibleTo<Val, V>)
    constexpr operator InValueResult<I, V>() && {
        return { util::move(in), util::move(value) };
    }

    [[no_unique_address]] In in;
    [[no_unique_address]] Val value;
};
}
