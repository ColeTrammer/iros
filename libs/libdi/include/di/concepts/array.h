#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/true_type.h>
#include <di/types/size_t.h>

namespace di::vocab {
template<typename T, types::size_t size>
struct Array;
}

namespace di::concepts {
namespace detail {
    template<typename T>
    struct ArrayHelper : meta::FalseType {};

    template<typename T, types::size_t size>
    struct ArrayHelper<vocab::Array<T, size>> : meta::TrueType {};
}

template<typename T>
concept Array = detail::ArrayHelper<meta::RemoveCVRef<T>>::value;
}
