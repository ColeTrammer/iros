#pragma once

#include <di/meta/remove_cvref.h>
#include <di/types/prelude.h>
#include <di/vocab/md/extents_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool extents_helper = false;

    template<typename T, usize... ins>
    constexpr inline bool extents_helper<vocab::Extents<T, ins...>> = true;
}

template<typename T>
concept Extents = detail::extents_helper<meta::RemoveCVRef<T>>;
}
