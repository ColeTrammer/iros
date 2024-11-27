#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/distance.h>
#include <di/container/meta/prelude.h>
#include <di/function/compare.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/tuple/prelude.h>

namespace di::container {
namespace detail {
    struct PopHeapFunction {
        template<concepts::RandomAccessIterator It, concepts::SentinelFor<It> Sent, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<It, Comp, Proj>)
        constexpr auto operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const -> It {
            return impl(util::move(first), util::ref(comp), util::ref(proj), container::distance(first, last));
        }

        template<concepts::RandomAccessContainer Con, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<meta::ContainerIterator<Con>, Comp, Proj>)
        constexpr auto operator()(Con&& container, Comp comp = {}, Proj proj = {}) const
            -> meta::BorrowedIterator<Con> {
            return impl(container::begin(container), util::ref(comp), util::ref(proj), container::distance(container));
        }

    private:
        friend struct MakeHeapFunction;

        constexpr static void bubble_down(auto first, auto comp, auto proj, auto size, decltype(size) index) {
            using IndexType = decltype(size);

            auto child_indices = [&](auto index) -> Tuple<Optional<IndexType>, Optional<IndexType>> {
                auto left_index = 2 * (index + 1) - 1;
                auto right_index = 2 * (index + 1);

                auto maybe_index = [&](IndexType index) -> Optional<IndexType> {
                    if (index >= size) {
                        return nullopt;
                    }
                    return index;
                };
                return { maybe_index(left_index), maybe_index(right_index) };
            };

            for (;;) {
                auto [left_child, right_child] = child_indices(index);
                if (!left_child) {
                    break;
                }
                if (!right_child) {
                    if (function::invoke(comp, function::invoke(proj, first[*left_child]),
                                         function::invoke(proj, first[index])) > 0) {
                        container::iterator_swap(first + *left_child, first + index);
                    }
                    break;
                }

                auto largest_child = function::invoke(comp, function::invoke(proj, first[*left_child]),
                                                      function::invoke(proj, first[*right_child])) > 0
                                         ? *left_child
                                         : *right_child;
                if (function::invoke(comp, function::invoke(proj, first[index]),
                                     function::invoke(proj, first[largest_child])) > 0) {
                    break;
                }
                container::iterator_swap(first + index, first + largest_child);
                index = largest_child;
            }
        }

        constexpr static auto impl(auto first, auto comp, auto proj, auto size) {
            auto last = first + size;
            container::iterator_swap(first, first + --size);
            bubble_down(first, util::ref(comp), util::ref(proj), size, 0);
            return last;
        }
    };
}

constexpr inline auto pop_heap = detail::PopHeapFunction {};
}

namespace di {
using container::pop_heap;
}
