#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
struct SizeFunction {
    template<Container C>
    constexpr size_t operator()(C&& container) const {
        if constexpr (SizedContainer<C>) {
            return container.size();
        } else {
            size_t result = 0;
            auto end = container.end();
            for (auto it = container.begin(); it != end; ++it) {
                result++;
            }
            return result;
        }
    }
};

constexpr inline auto size = SizeFunction {};
}
