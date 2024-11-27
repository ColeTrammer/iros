#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/meta/trivial.h>
#include <di/util/addressof.h>
#include <di/util/default_construct_at.h>

namespace di::container {
namespace detail {
    struct UninitializedDefaultConstructNFunction {
        template<concepts::UninitForwardIterator Out>
        requires(concepts::DefaultInitializable<meta::IteratorValue<Out>>)
        constexpr auto operator()(Out out, meta::IteratorSSizeType<Out> n) const -> Out {
            if constexpr (concepts::TriviallyDefaultConstructible<meta::IteratorValue<Out>>) {
                return container::next(out, n);
            }

            for (; n > 0; --n, ++out) {
                util::default_construct_at(util::addressof(*out));
            }
            return out;
        }
    };
}

constexpr inline auto uninitialized_default_construct_n = detail::UninitializedDefaultConstructNFunction {};
}

namespace di {
using container::uninitialized_default_construct_n;
}
