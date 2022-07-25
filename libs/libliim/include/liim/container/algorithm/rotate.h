#pragma once

#include <liim/container/concepts.h>
#include <liim/container/producer/iterator_container.h>

namespace LIIM::Container::Algorithm {
struct RotateFunction {
    template<MutableRandomAccessContainer C>
    constexpr auto operator()(C&& container, IteratorForContainer<C> middle) const {
        auto start = forward<C>(container).begin();
        auto end = forward<C>(container).end();

        auto to_rotate = end - middle;
        reverse(forward<C>(container));
        reverse(iterator_container(start, start + to_rotate));
        reverse(iterator_container(start + to_rotate, end));
        return start + to_rotate;
    }
};

constexpr inline auto rotate = RotateFunction {};
}
