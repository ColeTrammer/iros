#pragma once

#include <di/concepts/trivially_destructible.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/util/addressof.h>
#include <di/util/destroy_at.h>

namespace di::container {
namespace detail {
    struct DestroyNFunction {
        template<concepts::UninitInputIterator It>
        requires(concepts::Destructible<meta::IteratorValue<It>>)
        constexpr It operator()(It it, meta::IteratorSSizeType<It> n) const {
            if constexpr (concepts::TriviallyDestructible<meta::IteratorValue<It>>) {
                container::advance(it, n);
                return it;
            } else {
                for (; n != 0; --n) {
                    util::destroy_at(util::addressof(*it));
                    ++it;
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

constexpr inline auto destroy_n = detail::DestroyNFunction {};
}
