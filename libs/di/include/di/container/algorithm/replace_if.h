#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct ReplaceIfFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, typename U,
                 typename Proj = function::Identity, concepts::IndirectUnaryPredicate<meta::Projected<It, Proj>> Pred>
        requires(concepts::IndirectlyWritable<It, U const&>)
        constexpr It operator()(It first, Sent last, Pred pred, U const& new_value, Proj proj = {}) const {
            for (; first != last; ++first) {
                if (function::invoke(pred, function::invoke(proj, *first))) {
                    *first = new_value;
                }
            }
            return first;
        }

        template<concepts::InputContainer Con, typename U, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        requires(concepts::IndirectlyWritable<meta::ContainerIterator<Con>, U const&>)
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, Pred pred, U const& new_value,
                                                         Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), new_value, proj);
        }
    };
}

constexpr inline auto replace_if = detail::ReplaceIfFunction {};
}

namespace di {
using container::replace_if;
}
