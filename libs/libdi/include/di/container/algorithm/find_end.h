#pragma once

#include <di/container/algorithm/mismatch.h>
#include <di/container/algorithm/search.h>
#include <di/container/iterator/next.h>
#include <di/container/iterator/reverse_iterator.h>
#include <di/container/view/view.h>

namespace di::container {
namespace detail {
    struct FindEndFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, concepts::ForwardIterator Jt,
                 concepts::SentinelFor<Jt> Jent, typename Pred = function::Equal, typename Proj = function::Identity,
                 typename Jroj = function::Identity>
        requires(concepts::IndirectlyComparable<It, Jt, Pred, Proj, Jroj>)
        constexpr View<It> operator()(It it, Sent ed, Jt jt, Jent fd, Pred pred = {}, Proj proj = {},
                                      Jroj jroj = {}) const {
            if (jt == fd) {
                auto last = container::next(it, ed);
                return { last, last };
            }
            if constexpr (concepts::BidirectionalIterator<It> && concepts::SameAs<It, Sent> &&
                          concepts::BidirectionalIterator<Jt> && concepts::SameAs<Jt, Jent>) {
                auto result =
                    container::search(make_reverse_iterator(ed), make_reverse_iterator(it), make_reverse_iterator(fd),
                                      make_reverse_iterator(jt), util::ref(pred), util::ref(proj), util::ref(jroj));
                if (!result) {
                    return { ed, ed };
                }
                auto [a, b] = result;
                return { b.base(), a.base() };
            } else {
                auto r1 = It {};
                auto r2 = It {};
                for (; it != ed; ++it) {
                    auto result =
                        container::mismatch(it, ed, jt, fd, util::ref(pred), util::ref(proj), util::ref(jroj));
                    if (result.in2 == fd) {
                        r1 = it;
                        r2 = util::move(result.in1);
                    }
                }
                if (r1 == It {} && r2 == It {}) {
                    return { it, it };
                }
                return { util::move(r1), util::move(r2) };
            }
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

constexpr inline auto find_end = detail::FindEndFunction {};
}