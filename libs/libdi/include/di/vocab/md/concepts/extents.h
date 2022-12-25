#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/true_type.h>
#include <di/vocab/md/extents_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct ExtentsHelper : meta::FalseType {};

    template<typename T, size_t... ins>
    struct ExtentsHelper<vocab::Extents<T, ins...>> : meta::TrueType {};
}

template<typename T>
concept Extents = detail::ExtentsHelper<meta::RemoveCVRef<T>>::value;
}