#pragma once

#include "di/container/concepts/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/function/equal.h"
#include "di/function/identity.h"
#include "di/function/invoke.h"
#include "di/util/reference_wrapper.h"

namespace di::container {
namespace detail {
    struct FindFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename T,
                 typename Proj = function::Identity>
        requires(concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<Iter, Proj>, T const*>)
        constexpr auto operator()(Iter first, Sent last, T const& needle, Proj proj = {}) const -> Iter {
            for (; first != last; ++first) {
                if (function::invoke(proj, *first) == needle) {
                    break;
                }
            }
            return first;
        }

        template<concepts::InputContainer Con, typename T, typename Proj = function::Identity>
        requires(concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<meta::ContainerIterator<Con>, Proj>,
                                                   T const*>)
        constexpr auto operator()(Con&& container, T const& needle, Proj proj = {}) const
            -> meta::BorrowedIterator<Con> {
            return (*this)(container::begin(container), container::end(container), needle, util::ref(proj));
        }
    };
}

constexpr inline auto find = detail::FindFunction {};
}

namespace di {
using container::find;
}
