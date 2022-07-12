#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/swap_iterator_contents.h>

namespace LIIM::Container::Algorithm {
template<MutableDoubleEndedContainer C>
constexpr void reverse(C&& container) {
    auto left = forward<C>(container).begin();
    auto right = forward<C>(container).end();

    while (left != right) {
        --right;
        if (left == right) {
            break;
        }
        swap_iterator_contents(left, right);
        ++left;
    }
}
}

using LIIM::Container::Algorithm::reverse;
