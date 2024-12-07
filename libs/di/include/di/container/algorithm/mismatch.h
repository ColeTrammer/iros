#pragma once

#include "di/container/algorithm/in_in_result.h"
#include "di/container/concepts/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/function/equal.h"
#include "di/function/identity.h"
#include "di/util/move.h"
#include "di/util/reference_wrapper.h"

namespace di::container {
namespace detail {
    struct MismatchFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::InputIterator Jt,
                 concepts::SentinelFor<Jt> Jent, typename Pred = function::Equal, typename Proj = function::Identity,
                 typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<It, Jt, Pred, Proj, Jroj>)
        constexpr auto operator()(It it, Sent ed, Jt jt, Jent fd, Pred pred = {}, Proj proj = {}, Jroj jroj = {}) const
            -> InInResult<It, Jt> {
            for (; it != ed && jt != fd; ++it, ++jt) {
                if (!function::invoke(pred, function::invoke(proj, *it), function::invoke(jroj, *jt))) {
                    break;
                }
            }
            return { util::move(it), util::move(jt) };
        }

        template<concepts::InputContainer Con, concepts::InputContainer Jon, typename Pred = function::Equal,
                 typename Proj = function::Identity, typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<meta::ContainerIterator<Con>, meta::ContainerIterator<Jon>, Pred, Proj,
                                                Jroj>)
        constexpr auto operator()(Con&& con, Jon&& jon, Pred pred = {}, Proj proj = {}, Jroj jroj = {}) const
            -> InInResult<meta::BorrowedIterator<Con>, meta::BorrowedIterator<Jon>> {
            return (*this)(container::begin(con), container::end(con), container::begin(jon), container::end(jon),
                           util::ref(pred), util::ref(proj), util::ref(jroj));
        }
    };
}

constexpr inline auto mismatch = detail::MismatchFunction {};
}

namespace di {
using container::mismatch;
}
