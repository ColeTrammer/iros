#pragma once

#include <di/meta/trivial.h>

namespace di::vocab {
template<concepts::TriviallyRelocatable T>
struct Erased {
    using Value = T;
};
}
