#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/move.h>

namespace di::container {
template<typename In, typename Out>
struct InOutResult {
    template<typename I, typename O>
    requires(concepts::ConvertibleTo<I const&, In> && concepts::ConvertibleTo<O const&, Out>)
    constexpr operator InOutResult<I, O>() const& {
        return { in, out };
    }

    template<typename I, typename O>
    requires(concepts::ConvertibleTo<I, In> && concepts::ConvertibleTo<O, Out>)
    constexpr operator InOutResult<I, O>() && {
        return { util::move(in), util::move(out) };
    }

    [[no_unique_address]] In in;
    [[no_unique_address]] Out out;
};
}
