#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/move.h>

template<typename In, typename Val>
struct InValueResult {
    template<typename I, typename V>
    requires(concepts::ConvertibleTo<I const&, In> && concepts::ConvertibleTo<V const&, Val>)
    constexpr operator InValueResult<I, V>() const& {
        return { in, value };
    }

    template<typename I, typename V>
    requires(concepts::ConvertibleTo<I, In> && concepts::ConvertibleTo<V, Val>)
    constexpr operator InValueResult<I, V>() && {
        return { util::move(in), util::move(val) };
    }

    [[no_unique_address]] In in;
    [[no_unique_address]] Val value;
};