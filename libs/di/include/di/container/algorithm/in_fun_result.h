#pragma once

#include "di/meta/operations.h"
#include "di/util/move.h"

namespace di::container {
template<typename In, typename F>
struct InFunResult {
    [[no_unique_address]] In in;
    [[no_unique_address]] F fun;

    template<typename Jn, typename G>
    requires(concepts::ConvertibleTo<In const&, Jn> && concepts::ConvertibleTo<F const&, G>)
    constexpr operator InFunResult<Jn, G>() const& {
        return { in, fun };
    }

    template<typename Jn, typename G>
    requires(concepts::ConvertibleTo<In, Jn> && concepts::ConvertibleTo<F, G>)
    constexpr operator InFunResult<Jn, G>() && {
        return { util::move(in), util::move(fun) };
    }
};
}
