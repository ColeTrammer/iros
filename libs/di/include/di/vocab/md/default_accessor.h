#pragma once

#include <di/concepts/convertible_to_non_slicing.h>
#include <di/types/prelude.h>

namespace di::vocab {
template<typename Element>
class DefaultAccessor {
public:
    using OffsetPolicy = DefaultAccessor;
    using ElementType = Element;
    using Reference = ElementType&;
    using DataHandle = ElementType*;

    DefaultAccessor() = default;

    template<concepts::ConvertibleToNonSlicing<Element> OtherElement>
    constexpr DefaultAccessor(DefaultAccessor<OtherElement>) {}

    constexpr Reference access(DataHandle p, size_t i) const { return p[i]; }

    constexpr DataHandle offset(DataHandle p, size_t i) const { return p + i; }
};
}
