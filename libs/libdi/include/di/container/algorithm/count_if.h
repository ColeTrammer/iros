#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/identity.h>
#include <di/function/invoke.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
namespace detail {
    struct CountIfFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<Iter, Proj>> Pred>
        constexpr meta::IteratorSSizeType<Iter> operator()(Iter first, Sent last, Pred pred,
                                                           Proj projection = {}) const {
            auto result = meta::IteratorSSizeType<Iter> { 0 };
            for (; first != last; ++first) {
                if (function::invoke(pred, function::invoke(projection, *first))) {
                    result++;
                }
            }
            return result;
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        constexpr meta::ContainerSSizeType<Con> operator()(Con&& container, Pred pred, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto count_if = detail::CountIfFunction {};
}