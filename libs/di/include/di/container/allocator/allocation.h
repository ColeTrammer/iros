#pragma once

#include <di/types/prelude.h>

namespace di::container {
template<typename T>
struct Allocation {
    using Value = T;

    T* data { nullptr };
    size_t count { 0 };
};
}
