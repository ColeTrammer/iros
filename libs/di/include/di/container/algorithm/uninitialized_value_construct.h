#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/meta/trivial.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>

namespace di::container {
namespace detail {
    struct UninitializedValueConstructFunction {
        template<concepts::UninitForwardIterator Out, concepts::UninitSentinelFor<Out> OutSent>
        requires(concepts::DefaultInitializable<meta::IteratorValue<Out>>)
        constexpr Out operator()(Out out, OutSent out_last) const {
            for (; out != out_last; ++out) {
                util::construct_at(util::addressof(*out));
            }
            return out;
        }

        template<concepts::UninitForwardContainer Out>
        requires(concepts::DefaultInitializable<meta::ContainerValue<Out>>)
        constexpr meta::BorrowedIterator<Out> operator()(Out&& out) const {
            return (*this)(container::begin(out), container::end(out));
        }
    };
}

constexpr inline auto uninitialized_value_construct = detail::UninitializedValueConstructFunction {};
}
