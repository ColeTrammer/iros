#pragma once

#include <di/container/algorithm/search.h>

namespace di::container {
namespace detail {
    struct ContainsSubrangeFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, concepts::ForwardIterator Jt,
                 concepts::SentinelFor<Jt> Jent, typename Pred = function::Equal, typename Proj = function::Identity,
                 typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<It, Jt, Pred, Proj, Jroj>)
        constexpr bool operator()(It it, Sent ed, Jt jt, Jent fd, Pred pred = {}, Proj proj = {},
                                  Jroj jroj = {}) const {
            return jt == fd ||
                   !container::search(it, ed, jt, fd, util::ref(pred), util::ref(proj), util::ref(jroj)).empty();
        }

        template<concepts::InputContainer Con, concepts::InputContainer Jon, typename Pred = function::Equal,
                 typename Proj = function::Identity, typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<meta::ContainerIterator<Con>, meta::ContainerIterator<Jon>, Pred, Proj,
                                                Jroj>)
        constexpr bool operator()(Con&& con, Jon&& jon, Pred pred = {}, Proj proj = {}, Jroj jroj = {}) const {
            return (*this)(container::begin(con), container::end(con), container::begin(jon), container::end(jon),
                           util::ref(pred), util::ref(proj), util::ref(jroj));
        }
    };
}

constexpr inline auto contains_subrange = detail::ContainsSubrangeFunction {};
}
