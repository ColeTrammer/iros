#pragma once

#include <di/container/algorithm/in_found_result.h>
#include <di/container/algorithm/lower_bound.h>

namespace di::container {
namespace detail {
    struct BinarySearchFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, typename T,
                 typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<T const*, meta::Projected<It, Proj>> Comp = function::Compare>
        constexpr InFoundResult<It> operator()(It first, Sent last, T const& needle, Comp comp = {},
                                               Proj proj = {}) const {
            auto const distance = container::distance(first, last);
            return binary_search_with_size(util::move(first), needle, util::ref(comp), util::ref(proj), distance);
        }

        template<concepts::ForwardContainer Con, typename T, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<T const*, meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Compare>
        constexpr InFoundResult<meta::BorrowedIterator<Con>> operator()(Con&& container, T const& needle,
                                                                        Comp comp = {}, Proj proj = {}) const {
            auto const distance = container::distance(container);
            return binary_search_with_size(container::begin(container), needle, util::ref(comp), util::ref(proj),
                                           distance);
        }

    private:
        template<typename It, typename T, typename Proj, typename Comp,
                 typename SSizeType = meta::IteratorSSizeType<It>>
        constexpr static InFoundResult<It> binary_search_with_size(It first, T const& needle, Comp comp, Proj proj,
                                                                   meta::TypeIdentity<SSizeType> n) {
            while (n != 0) {
                SSizeType left_length = n >> 1;
                auto mid = container::next(first, left_length);
                auto result = function::invoke(comp, needle, function::invoke(proj, *mid));
                if (result == 0) {
                    // found the needle, return true.
                    return { util::move(mid), true };
                } else if (result > 0) {
                    // needle is greater than every element in the range [first, mid].
                    n -= left_length + 1;
                    first = ++mid;
                } else {
                    // needle is less than every element in the range [mid, last).
                    n = left_length;
                }
            }

            return { util::move(first), false };
        }
    };
}

constexpr inline auto binary_search = detail::BinarySearchFunction {};
}

namespace di {
using container::binary_search;
}
