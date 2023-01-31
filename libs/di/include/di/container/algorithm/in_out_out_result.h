#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/move.h>

namespace di::container {
template<typename In, typename Out1, typename OUt2>
struct InOutOutResult {
    template<typename I, typename O1, typename O2>
    requires(concepts::ConvertibleTo<In const&, I> && concepts::ConvertibleTo<Out1 const&, O1> &&
             concepts::ConvertibleTo<OUt2 const&, O2>)
    constexpr operator InOutOutResult<I, O1, O2>() const& {
        return { in, out1, out2 };
    }

    template<typename I, typename O1, typename O2>
    requires(concepts::ConvertibleTo<In, I> && concepts::ConvertibleTo<Out1, O1> && concepts::ConvertibleTo<OUt2, O2>)
    constexpr operator InOutOutResult<I, O1, O2>() && {
        return { util::move(in), util::move(out1), util::move(out2) };
    }

    [[no_unique_address]] In in;
    [[no_unique_address]] Out1 out1;
    [[no_unique_address]] OUt2 out2;
};
}
