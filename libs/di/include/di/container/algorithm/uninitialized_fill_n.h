#pragma once

#include "di/container/concepts/prelude.h"
#include "di/container/iterator/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/util/addressof.h"
#include "di/util/construct_at.h"

namespace di::container {
namespace detail {
    struct UninitializedFillNFunction {
        template<concepts::UninitForwardIterator Out, concepts::UninitSentinelFor<Out> OutSent, typename T>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Out>, T const&>)
        constexpr auto operator()(Out out, meta::IteratorSSizeType<Out> n, T const& value) const -> Out {
            for (; n > 0; --n, ++out) {
                util::construct_at(util::addressof(*out), value);
            }
            return out;
        }
    };
}

constexpr inline auto uninitialized_fill_n = detail::UninitializedFillNFunction {};
}

namespace di {
using container::uninitialized_fill_n;
}
