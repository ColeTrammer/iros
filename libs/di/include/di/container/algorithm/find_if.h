#pragma once

#include "di/container/concepts/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/function/identity.h"
#include "di/function/invoke.h"
#include "di/util/reference_wrapper.h"

namespace di::container {
namespace detail {
    struct FindIfFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<Iter, Proj>> Pred>
        constexpr auto operator()(Iter first, Sent last, Pred pred, Proj projection = {}) const -> Iter {
            for (; first != last; ++first) {
                if (function::invoke(pred, function::invoke(projection, *first))) {
                    break;
                }
            }
            return first;
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        constexpr auto operator()(Con&& container, Pred pred, Proj proj = {}) const -> meta::BorrowedIterator<Con> {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto find_if = detail::FindIfFunction {};
}

namespace di {
using container::find_if;
}
