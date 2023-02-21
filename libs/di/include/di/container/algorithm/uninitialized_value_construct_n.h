#pragma once

#include <di/concepts/trivially_default_constructible.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>

namespace di::container {
namespace detail {
    struct UninitializedValueConstructNFunction {
        template<concepts::UninitForwardIterator Out>
        requires(concepts::DefaultInitializable<meta::IteratorValue<Out>>)
        constexpr Out operator()(Out out, meta::IteratorSSizeType<Out> n) const {
            for (; n > 0; --n, ++out) {
                util::construct_at(util::addressof(*out));
            }
            return out;
        }
    };
}

constexpr inline auto uninitialized_value_construct_n = detail::UninitializedValueConstructNFunction {};
}
