#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/move.h>

namespace di::container {
template<typename In, typename Out>
struct InOutResult {
    template<typename I, typename O>
    requires(concepts::ConvertibleTo<In const&, I> && concepts::ConvertibleTo<Out const&, O>)
    constexpr operator InOutResult<I, O>() const& {
        return { in, out };
    }

    template<typename I, typename O>
    requires(concepts::ConvertibleTo<In, I> && concepts::ConvertibleTo<Out, O>)
    constexpr operator InOutResult<I, O>() && {
        return { util::move(in), util::move(out) };
    }

    [[no_unique_address]] In in;
    [[no_unique_address]] Out out;
};
}
