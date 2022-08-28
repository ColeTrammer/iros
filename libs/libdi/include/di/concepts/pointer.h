#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cv.h>
#include <di/meta/true_type.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct PointerHelper : meta::FalseType {};

    template<typename T>
    struct PointerHelper<T*> : meta::TrueType {};
}

template<typename T>
concept Pointer = detail::PointerHelper<meta::RemoveCV<T>>::value;
}
