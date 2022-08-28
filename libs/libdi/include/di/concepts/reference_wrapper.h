#pragma once

#include <di/meta/false_type.h>
#include <di/meta/true_type.h>

namespace di::util {
template<typename T>
class ReferenceWrapper;
}

namespace di::concepts {
namespace detail {
    template<typename T>
    struct ReferenceWrapperHelper : meta::FalseType {};

    template<typename T>
    struct ReferenceWrapperHelper<di::util::ReferenceWrapper<T>> : meta::TrueType {};
}

template<typename T>
concept ReferenceWrapper = detail::ReferenceWrapperHelper<meta::RemoveCV<T>>::value;
}
