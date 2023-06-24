#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/meta/trivial.h>
#include <di/util/addressof.h>
#include <di/util/destroy_at.h>

namespace di::container {
namespace detail {
    struct DestroyFunction {
        template<concepts::UninitInputIterator It, concepts::UninitSentinelFor<It> Sent>
        requires(concepts::Destructible<meta::IteratorValue<It>>)
        constexpr It operator()(It it, Sent sent) const {
            if constexpr (concepts::TriviallyDestructible<meta::IteratorValue<It>>) {
                container::advance(it, sent);
                return it;
            } else {
                for (; it != sent; ++it) {
                    util::destroy_at(util::addressof(*it));
                }
                return it;
            }
        }

        template<concepts::UninitInputContainer Con>
        requires(concepts::Destructible<meta::ContainerValue<Con>>)
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container) const {
            return (*this)(container::begin(container), container::end(container));
        }
    };
}

constexpr inline auto destroy = detail::DestroyFunction {};
}
