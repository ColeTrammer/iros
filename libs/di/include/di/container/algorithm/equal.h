#pragma once

#include <di/container/algorithm/mismatch.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/distance.h>
#include <di/container/meta/prelude.h>
#include <di/function/equal.h>
#include <di/function/identity.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
namespace detail {
    struct EqualFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::InputIterator Jt,
                 concepts::SentinelFor<Jt> Jent, typename Pred = function::Equal, typename Proj = function::Identity,
                 typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<It, Jt, Pred, Proj, Jroj>)
        constexpr auto operator()(It it, Sent ed, Jt jt, Jent fd, Pred pred = {}, Proj proj = {}, Jroj jroj = {}) const
            -> bool {
            if constexpr (concepts::SizedSentinelFor<Sent, It> && concepts::SizedSentinelFor<Jt, Jent>) {
                if (container::distance(it, ed) != container::distance(jt, fd)) {
                    return false;
                }
            }
            auto [lt, kt] =
                mismatch(util::move(it), ed, util::move(jt), fd, util::ref(pred), util::ref(proj), util::ref(jroj));
            return lt == ed && kt == fd;
        }

        template<concepts::InputContainer Con, concepts::InputContainer Jon, typename Pred = function::Equal,
                 typename Proj = function::Identity, typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<meta::ContainerIterator<Con>, meta::ContainerIterator<Jon>, Pred, Proj,
                                                Jroj>)
        constexpr auto operator()(Con&& con, Jon&& jon, Pred pred = {}, Proj proj = {}, Jroj jroj = {}) const -> bool {
            if constexpr (concepts::SizedContainer<Con> && concepts::SizedContainer<Jon>) {
                if (container::size(con) != container::size(jon)) {
                    return false;
                }
            }
            return (*this)(container::begin(con), container::end(con), container::begin(jon), container::end(jon),
                           util::ref(pred), util::ref(proj), util::ref(jroj));
        }
    };
}

constexpr inline auto equal = detail::EqualFunction {};
}
