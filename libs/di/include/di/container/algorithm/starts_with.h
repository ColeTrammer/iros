#pragma once

#include <di/container/algorithm/mismatch.h>

namespace di::container {
namespace detail {
    struct StartsWithFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::InputIterator Jt,
                 concepts::SentinelFor<Jt> Jent, typename Pred = function::Equal, typename Proj = function::Identity,
                 typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<It, Jt, Pred, Proj, Jroj>)
        constexpr auto operator()(It it, Sent ed, Jt jt, Jent fd, Pred pred = {}, Proj proj = {}, Jroj jroj = {}) const
            -> bool {
            return container::mismatch(util::move(it), ed, util::move(jt), fd, util::ref(pred), util::ref(proj),
                                       util::ref(jroj))
                       .in2 == fd;
        }

        template<concepts::InputContainer Con, concepts::InputContainer Jon, typename Pred = function::Equal,
                 typename Proj = function::Identity, typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<meta::ContainerIterator<Con>, meta::ContainerIterator<Jon>, Pred, Proj,
                                                Jroj>)
        constexpr auto operator()(Con&& con, Jon&& jon, Pred pred = {}, Proj proj = {}, Jroj jroj = {}) const -> bool {
            return (*this)(container::begin(con), container::end(con), container::begin(jon), container::end(jon),
                           util::ref(pred), util::ref(proj), util::ref(jroj));
        }
    };
}

constexpr inline auto starts_with = detail::StartsWithFunction {};
}

namespace di {
using container::starts_with;
}
