#pragma once

#include <di/meta/core.h>
#include <di/vocab/error/erased.h>
#include <di/vocab/error/status_code_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool erased_status_code_helper = false;

    template<typename T>
    constexpr inline bool erased_status_code_helper<vocab::StatusCode<vocab::Erased<T>>> = true;
}

template<typename T>
concept ErasedStatusCode = detail::erased_status_code_helper<meta::Decay<T>>;
}
