#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/equal.h>
#include <di/function/identity.h>
#include <di/function/invoke.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
namespace detail {
    struct CountFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename T,
                 typename Proj = function::Identity>
        requires(concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<Iter, Proj>, T const*>)
        constexpr meta::IteratorSSizeType<Iter> operator()(Iter first, Sent last, T const& needle,
                                                           Proj proj = {}) const {
            auto result = meta::IteratorSSizeType<Iter> { 0 };
            for (; first != last; ++first) {
                if (function::invoke(proj, *first) == needle) {
                    result++;
                }
            }
            return result;
        }

        template<concepts::InputContainer Con, typename T, typename Proj = function::Identity>
        requires(concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<meta::ContainerIterator<Con>, Proj>,
                                                   T const*>)
        constexpr meta::ContainerSSizeType<Con> operator()(Con&& container, T const& needle, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), needle, util::ref(proj));
        }
    };
}

constexpr inline auto count = detail::CountFunction {};
}