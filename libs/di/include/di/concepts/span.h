#pragma once

#include <di/meta/remove_cvref.h>
#include <di/types/prelude.h>
#include <di/vocab/span/span_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool span_helper = false;

    template<typename T, usize extent>
    constexpr inline bool span_helper<vocab::Span<T, extent>> = true;
}

template<typename T>
concept Span = detail::span_helper<meta::RemoveCVRef<T>>;
}
