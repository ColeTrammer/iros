#pragma once

#include <di/concepts/integer.h>
#include <di/math/to_unsigned.h>
#include <di/types/size_t.h>
#include <di/vocab/span/span_forward_declaration.h>

namespace di::vocab {
template<concepts::Integer T, types::size_t... extents>
requires(
    concepts::Conjunction<(extents == dynamic_extent || extents <= math::to_unsigned(math::NumericLimits<T>::max))...>)
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