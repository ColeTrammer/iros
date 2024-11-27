#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/meta/trivial.h>
#include <di/util/addressof.h>
#include <di/util/destroy_at.h>

namespace di::container {
namespace detail {
    struct DestroyNFunction {
        template<concepts::UninitInputIterator It>
        requires(concepts::Destructible<meta::IteratorValue<It>>)
        constexpr auto operator()(It it, meta::IteratorSSizeType<It> n) const -> It {
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
        constexpr auto operator()(Con&& container) const -> meta::BorrowedIterator<Con> {
            return (*this)(container::begin(container), container::end(container));
        }
    };
}

constexpr inline auto destroy_n = detail::DestroyNFunction {};
}

namespace di {
using container::destroy_n;
}
