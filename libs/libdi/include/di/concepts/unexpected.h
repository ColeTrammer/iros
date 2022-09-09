#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cv.h>
#include <di/meta/true_type.h>
#include <di/vocab/expected/expected_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct UnexpectedHelper : meta::FalseType {};

    template<typename T>
    struct UnexpectedHelper<vocab::Unexpected<T>> : meta::TrueType {};
}

template<typename T>
concept Unexpected = detail::UnexpectedHelper<meta::RemoveCV<T>>::value;
}
