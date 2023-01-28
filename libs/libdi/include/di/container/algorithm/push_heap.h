#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/distance.h>
#include <di/container/meta/prelude.h>
#include <di/function/compare.h>

namespace di::container {
namespace detail {
    struct PushHeapFunction {
        template<concepts::RandomAccessIterator It, concepts::SentinelFor<It> Sent, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<It, Comp, Proj>)
        constexpr It operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const {
            return this->impl(util::move(first), util::ref(comp), util::ref(proj), container::distance(first, last));
        }

        template<concepts::RandomAccessContainer Con, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<meta::ContainerIterator<Con>, Comp, Proj>)
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, Comp comp = {}, Proj proj = {}) const {
            return impl(container::begin(container), util::ref(comp), util::ref(proj), container::distance(container));
        }

    private:
        constexpr static auto impl(auto first, auto comp, auto proj, auto size) {
            auto parent_index = [size](auto index) {
                return (index + 1) / 2 - 1;
            };

            auto index = size - 1;
            for (auto parent = parent_index(index);
                 index && function::invoke(comp, function::invoke(proj, first[index]),
                                           function::invoke(proj, first[parent])) > 0;
                 index = parent, parent = parent_index(index)) {
                container::iterator_swap(first + index, first + parent);
            }
            return first + size;
        }
    };
}

constexpr inline auto push_heap = detail::PushHeapFunction {};
}