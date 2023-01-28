#pragma once

#include <di/container/algorithm/equal.h>
#include <di/container/iterator/distance.h>

namespace di::container {
namespace detail {
    struct EndsWithFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::InputIterator Jt,
                 concepts::SentinelFor<Jt> Jent, typename Pred = function::Equal, typename Proj = function::Identity,
                 typename Jroj = function::Identity>
        requires((concepts::ForwardIterator<It> || concepts::SizedSentinelFor<Sent, It>) &&
                 (concepts::ForwardIterator<Jt> || concepts::SizedSentinelFor<Jent, Jt>) &&
                 concepts::IndirectlyComparable<It, Jt, Pred, Proj, Jroj>)
        constexpr bool operator()(It it, Sent ed, Jt jt, Jent fd, Pred pred = {}, Proj proj = {},
                                  Jroj jroj = {}) const {
            auto n = container::distance(it, ed);
            auto m = container::distance(jt, fd);
            if (n < m) {
                return false;
            }

            container::advance(it, n - m);
            return container::equal(util::move(it), ed, util::move(jt), fd, util::ref(pred), util::ref(proj),
                                    util::ref(jroj));
        }

        template<concepts::InputContainer Con, concepts::InputContainer Don, typename Pred = function::Equal,
                 typename Proj = function::Identity, typename Jroj = function::Identity>
        requires((concepts::ForwardContainer<Con> || concepts::SizedContainer<Con>) &&
                 (concepts::ForwardContainer<Don> || concepts::SizedContainer<Don>) &&
                 concepts::IndirectlyComparable<meta::ContainerIterator<Con>, meta::ContainerIterator<Don>, Pred, Proj,
                                                Jroj>)
        constexpr bool operator()(Con&& a, Don&& b, Pred pred = {}, Proj proj = {}, Jroj jroj = {}) const {
            return (*this)(container::begin(a), container::end(a), container::begin(b), container::end(b),
                           util::ref(pred), util::ref(proj), util::ref(jroj));
        }
    };
}

constexpr inline auto ends_with = detail::EndsWithFunction {};
}