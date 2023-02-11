#pragma once

#include <di/prelude.h>

namespace dius::filesystem {
namespace detail {
    struct IsEmptyFunction {
        di::Result<bool> operator()(di::PathView path) const;
    };
}

constexpr inline auto is_empty = detail::IsEmptyFunction {};
}