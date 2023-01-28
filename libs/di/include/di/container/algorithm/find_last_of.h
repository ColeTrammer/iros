#pragma once

#include <di/container/algorithm/any_of.h>
#include <di/container/algorithm/find_last_if.h>

namespace di::container {
namespace detail {
    struct FindLastOfFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::ForwardIterator Jt,
                 concepts::SentinelFor<Jt> Jent, typename Pred = function::Equal, typename Proj = function::Identity,
                 typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<It, Jt, Pred, Proj, Jroj>)
        constexpr View<It> operator()(It it, Sent sent, Jt jt, Jent jent, Pred pred = {}, Proj proj = {},
                                      Jroj jroj = {}) const {
            return container::find_last_if(
                util::move(it), sent,
                [&]<typename T>(T&& value) {
                    return container::any_of(
                        jt, jent,
                        [&]<typename U>(U&& other) {
                            return function::invoke(pred, value, other);
                        },
                        util::ref(jroj));
                },
                util::ref(proj));
        }

        template<concepts::InputContainer Con, concepts::ForwardContainer Needles, typename Pred = function::Equal,
                 typename Proj1 = function::Identity, typename Proj2 = function::Identity>
        requires(concepts::IndirectlyComparable<meta::ContainerIterator<Con>, meta::ContainerIterator<Needles>, Pred,
                                                Proj1, Proj2>)
        constexpr meta::BorrowedView<Con> operator()(Con&& container, Needles&& needles, Pred pred = {},
                                                     Proj1 proj1 = {}, Proj2 proj2 = {}) const {
            return (*this)(container::begin(container), container::end(container), container::begin(needles),
                           container::end(needles), util::ref(pred), util::ref(proj1), util::ref(proj2));
        }
    };
}

constexpr inline auto find_last_of = detail::FindLastOfFunction {};
}