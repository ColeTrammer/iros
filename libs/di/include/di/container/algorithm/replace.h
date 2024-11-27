#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/equal.h>

namespace di::container {
namespace detail {
    struct ReplaceFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, typename T, typename U,
                 typename Proj = function::Identity>
        requires(concepts::IndirectlyWritable<It, U const&> &&
                 concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<It, Proj>, T const*>)
        constexpr auto operator()(It first, Sent last, T const& old_value, U const& new_value, Proj proj = {}) const
            -> It {
            for (; first != last; ++first) {
                if (old_value == function::invoke(proj, *first)) {
                    *first = new_value;
                }
            }
            return first;
        }

        template<concepts::InputContainer Con, typename T, typename U, typename Proj = function::Identity>
        requires(concepts::IndirectlyWritable<meta::ContainerIterator<Con>, U const&> &&
                 concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<meta::ContainerIterator<Con>, Proj>,
                                                   T const*>)
        constexpr auto operator()(Con&& container, T const& old_value, U const& new_value, Proj proj = {}) const
            -> meta::BorrowedIterator<Con> {
            return (*this)(container::begin(container), container::end(container), old_value, new_value, proj);
        }
    };
}

constexpr inline auto replace = detail::ReplaceFunction {};
}

namespace di {
using container::replace;
}
