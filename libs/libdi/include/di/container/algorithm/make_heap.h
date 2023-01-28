#pragma once

#include <di/container/algorithm/pop_heap.h>

namespace di::container {
namespace detail {
    struct MakeHeapFunction {
        template<concepts::RandomAccessIterator It, concepts::SentinelFor<It> Sent, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<It, Comp, Proj>)
        constexpr It operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const {
            return impl(util::move(first), util::ref(comp), util::ref(proj), container::distance(first, last));
        }

        template<concepts::RandomAccessContainer Con, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<meta::ContainerIterator<Con>, Comp, Proj>)
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, Comp comp = {}, Proj proj = {}) const {
            return impl(container::begin(container), util::ref(comp), util::ref(proj), container::distance(container));
        }

    private:
        constexpr static auto impl(auto first, auto comp, auto proj, auto size) {
            for (auto index = size - 1; index >= 0; --index) {
                PopHeapFunction::bubble_down(first, util::ref(comp), util::ref(proj), size, index);
            }
            return first + size;
        }
    };
}

constexpr inline auto make_heap = detail::MakeHeapFunction {};
}