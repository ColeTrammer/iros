#pragma once

#include "di/math/to_unsigned.h"
#include "di/meta/language.h"
#include "di/types/size_t.h"
#include "di/vocab/span/span_forward_declaration.h"

namespace di::vocab {
template<concepts::Integer T, types::size_t... extents>
requires((extents == dynamic_extent || extents <= math::to_unsigned(math::NumericLimits<T>::max)) && ...)
class Extents;

struct LayoutLeft {
    template<typename Extents>
    class Mapping;
};

struct LayoutRight {
    template<typename Extents>
    class Mapping;
};

struct LayoutStride {
    template<typename Extents>
    class Mapping;
};
}

namespace di {
using vocab::Extents;
using vocab::LayoutLeft;
using vocab::LayoutRight;
using vocab::LayoutStride;
}
