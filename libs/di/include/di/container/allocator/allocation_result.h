#pragma once

#include <di/types/prelude.h>

namespace di::container {
template<typename T = void>
struct AllocationResult {
    T* data { nullptr };
    size_t count { 0 };
};
}

namespace di {
using container::AllocationResult;
}
