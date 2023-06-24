#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>

namespace di::container {
namespace detail {
    struct UninitializedFillFunction {
        template<concepts::UninitForwardIterator Out, concepts::UninitSentinelFor<Out> OutSent, typename T>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Out>, T const&>)
        constexpr Out operator()(Out out, OutSent out_last, T const& value) const {
            for (; out != out_last; ++out) {
                util::construct_at(util::addressof(*out), value);
            }
            return out;
        }

        template<concepts::UninitForwardContainer Out, typename T>
        requires(concepts::ConstructibleFrom<meta::ContainerValue<Out>, T const&>)
        constexpr meta::BorrowedIterator<Out> operator()(Out&& out, T const& value) const {
            return (*this)(container::begin(out), container::end(out), value);
        }
    };
}

constexpr inline auto uninitialized_fill = detail::UninitializedFillFunction {};
}

namespace di {
using container::uninitialized_fill;
}
