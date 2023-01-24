#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/move.h>

namespace di {
template<typename Out, typename Val>
struct OutValueResult {
    template<typename I, typename V>
    requires(concepts::ConvertibleTo<Out const&, I> && concepts::ConvertibleTo<Val const&, V>)
    constexpr operator OutValueResult<I, V>() const& {
        return { out, value };
    }

    template<typename I, typename V>
    requires(concepts::ConvertibleTo<Out, I> && concepts::ConvertibleTo<Val, V>)
    constexpr operator OutValueResult<I, V>() && {
        return { util::move(out), util::move(value) };
    }

    [[no_unique_address]] Out out;
    [[no_unique_address]] Val value;
};
}