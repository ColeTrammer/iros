#pragma once

#include <di/concepts/common_reference_with.h>
#include <di/concepts/copyable.h>
#include <di/concepts/same_as.h>

namespace di::concepts {
template<typename T>
concept MDAccessor =
    Copyable<T> &&
    requires {
        typename T::ElementType;
        typename T::DataHandle;
        typename T::Reference;
        typename T::OffsetPolicy;
        typename T::OffsetPolicy::DataHandle;
    } && Copyable<typename T::DataHandle> && CommonReferenceWith<typename T::Reference&&, typename T::ElementType&> &&
    requires(T const a, typename T::DataHandle p, size_t i) {
        { a.access(p, i) } -> SameAs<typename T::Reference>;
        { a.offset(p, i) } -> SameAs<typename T::OffsetPolicy::DataHandle>;
    };
}
