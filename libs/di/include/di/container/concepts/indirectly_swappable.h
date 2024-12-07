#pragma once

#include "di/container/iterator/iterator_swap.h"

namespace di::concepts {
template<typename T, typename U = T>
concept IndirectlySwappable = requires(T const a, U const b) {
    container::iterator_swap(a, a);
    container::iterator_swap(a, b);
    container::iterator_swap(b, a);
    container::iterator_swap(b, b);
};
}
