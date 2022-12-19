#pragma once

#include <di/concepts/trivially_relocatable.h>

namespace di::vocab {
template<concepts::TriviallyRelocatable T>
struct Erased {
    using Value = T;
};
}