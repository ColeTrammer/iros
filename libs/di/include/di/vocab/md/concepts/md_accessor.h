#pragma once

#include "di/meta/common.h"
#include "di/meta/core.h"
#include "di/meta/operations.h"

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
