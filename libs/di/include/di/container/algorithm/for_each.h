#pragma once

#include <di/container/algorithm/in_fun_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/identity.h>
#include <di/function/invoke.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
namespace detail {
    struct ForEachFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename Proj = function::Identity,
                 concepts::IndirectlyUnaryInvocable<meta::Projected<Iter, Proj>> F>
        constexpr auto operator()(Iter first, Sent last, F f, Proj proj = {}) const -> InFunResult<Iter, F> {
            for (; first != last; ++first) {
                function::invoke(f, function::invoke(proj, *first));
            }
            return { util::move(first), util::move(f) };
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectlyUnaryInvocable<meta::Projected<meta::ContainerIterator<Con>, Proj>> F>
        constexpr auto operator()(Con&& container, F f, Proj proj = {}) const
            -> InFunResult<meta::BorrowedIterator<Con>, F> {
            return (*this)(container::begin(container), container::end(container), util::move(f), util::ref(proj));
        }
    };
}

constexpr inline auto for_each = detail::ForEachFunction {};
}

namespace di {
using container::for_each;
}
