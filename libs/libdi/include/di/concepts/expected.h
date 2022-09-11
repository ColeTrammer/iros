#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/true_type.h>
#include <di/vocab/expected/expected_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct ExpectedHelper : meta::FalseType {};

    template<typename T, typename E>
    struct ExpectedHelper<di::vocab::Expected<T, E>> : meta::TrueType {};
}

template<typename T>
concept Expected = detail::ExpectedHelper<meta::RemoveCVRef<T>>::value;
}
