#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/move.h>

namespace di::container {
template<typename T>
struct MinMaxResult {
    template<typename U>
    requires(concepts::ConvertibleTo<T const&, U>)
    constexpr operator MinMaxResult<U>() const& {
        return { min, max };
    }

    template<typename U>
    requires(concepts::ConvertibleTo<T, U>)
    constexpr operator MinMaxResult<U>() && {
        return { util::move(min), util::move(max) };
    }

    [[no_unique_address]] T min;
    [[no_unique_address]] T max;
};
}
