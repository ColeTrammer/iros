#pragma once

#include "di/container/concepts/prelude.h"
#include "di/container/iterator/distance.h"
#include "di/container/meta/prelude.h"
#include "di/function/compare.h"
#include "di/function/identity.h"
#include "di/util/reference_wrapper.h"

namespace di::container {
namespace detail {
    struct CompareFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::InputIterator Jt,
                 concepts::SentinelFor<Jt> Jent, typename Comp = function::Compare, typename Proj = function::Identity,
                 typename Jroj = function::Identity>
        requires(concepts::IndirectStrictPartialOrder<Comp, meta::Projected<It, Proj>, meta::Projected<Jt, Jroj>>)
        constexpr auto operator()(It it, Sent ed, Jt jt, Jent fd, Comp comp = {}, Proj proj = {},
                                  Jroj jroj = {}) const {
            using Result = decltype(function::invoke(comp, function::invoke(proj, *it), function::invoke(jroj, *jt)));
            for (; it != ed && jt != fd; ++it, ++jt) {
                if (auto result = function::invoke(comp, function::invoke(proj, *it), function::invoke(jroj, *jt));
                    result != 0) {
                    return result;
                }
            }
            return static_cast<Result>((jt == fd) <=> (it == ed));
        }

        template<concepts::InputContainer Con, concepts::InputContainer Jon, typename Comp = function::Compare,
                 typename Proj = function::Identity, typename Jroj = function::Identity>
        requires(concepts::IndirectStrictPartialOrder<Comp, meta::Projected<meta::ContainerIterator<Con>, Proj>,
                                                      meta::Projected<meta::ContainerIterator<Jon>, Jroj>>)
        constexpr auto operator()(Con&& con, Jon&& jon, Comp comp = {}, Proj proj = {}, Jroj jroj = {}) const {
            return (*this)(container::begin(con), container::end(con), container::begin(jon), container::end(jon),
                           util::ref(comp), util::ref(proj), util::ref(jroj));
        }
    };
}

constexpr inline auto compare = detail::CompareFunction {};
}
