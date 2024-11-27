#pragma once

#include <di/meta/operations.h>
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

    constexpr auto access(DataHandle p, size_t i) const -> Reference { return p[i]; }

    constexpr auto offset(DataHandle p, size_t i) const -> DataHandle { return p + i; }
};
}

namespace di {
using vocab::DefaultAccessor;
}
