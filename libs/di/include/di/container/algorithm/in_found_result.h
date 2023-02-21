#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/move.h>

namespace di::container {
template<typename It>
struct InFoundResult {
    template<typename I>
    requires(concepts::ConvertibleTo<It const&, I>)
    constexpr operator InFoundResult<I>() const& {
        return { in, found };
    }

    template<typename I>
    requires(concepts::ConvertibleTo<It, I>)
    constexpr operator InFoundResult<I>() && {
        return { util::move(in), found };
    }

    [[no_unique_address]] It in;
    bool found;
};
}
