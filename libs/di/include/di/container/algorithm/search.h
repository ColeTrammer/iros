#pragma once

#include <di/container/algorithm/mismatch.h>
#include <di/container/view/view.h>

namespace di::container {
namespace detail {
    struct SearchFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, concepts::ForwardIterator Jt,
                 concepts::SentinelFor<Jt> Jent, typename Pred = function::Equal, typename Proj = function::Identity,
                 typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<It, Jt, Pred, Proj, Jroj>)
        constexpr View<It> operator()(It it, Sent ed, Jt jt, Jent fd, Pred pred = {}, Proj proj = {},
                                      Jroj jroj = {}) const {
            for (; it != ed; ++it) {
                auto result = container::mismatch(it, ed, jt, fd, util::ref(pred), util::ref(proj), util::ref(jroj));
                if (result.in2 == fd) {
                    return { it, result.in1 };
                }
            }
            return { it, it };
        }

        template<concepts::InputContainer Con, concepts::InputContainer Jon, typename Pred = function::Equal,
                 typename Proj = function::Identity, typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<meta::ContainerIterator<Con>, meta::ContainerIterator<Jon>, Pred, Proj,
                                                Jroj>)
        constexpr meta::BorrowedView<Con> operator()(Con&& con, Jon&& jon, Pred pred = {}, Proj proj = {},
                                                     Jroj jroj = {}) const {
            return (*this)(container::begin(con), container::end(con), container::begin(jon), container::end(jon),
                           util::ref(pred), util::ref(proj), util::ref(jroj));
        }
    };
}

constexpr inline auto search = detail::SearchFunction {};
}

namespace di {
using container::search;
}
