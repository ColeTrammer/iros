#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cv.h>
#include <di/meta/true_type.h>

namespace di::vocab::optional {
template<typename T>
class Optional;
}

namespace di::concepts {
namespace detail {
    template<typename T>
    struct OptionalHelper : meta::FalseType {};

    template<typename T>
    struct OptionalHelper<di::vocab::optional::Optional<T>> : meta::TrueType {};
}

template<typename T>
concept Optional = detail::OptionalHelper<meta::RemoveCV<T>>::value;
}
