#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/true_type.h>
#include <di/vocab/optional/optional_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct OptionalHelper : meta::FalseType {};

    template<typename T>
    struct OptionalHelper<di::vocab::Optional<T>> : meta::TrueType {};
}

template<typename T>
concept Optional = detail::OptionalHelper<meta::RemoveCVRef<T>>::value;
}
