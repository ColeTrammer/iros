#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
template<DoubleEndedContainer C>
constexpr void reverse(C&& container) {
    auto left = forward<C>(container).begin();
    auto right = forward<C>(container).end();

    while (left != right) {
        --right;
        if (left == right) {
            break;
        }
        swap(*left, *right);
        ++left;
    }
}
}

using LIIM::Container::Algorithm::reverse;
