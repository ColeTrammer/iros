#pragma once

#include <di/container/path/prelude.h>
#include <di/vocab/error/prelude.h>

namespace dius::filesystem {
namespace detail {
    struct IsEmptyFunction {
        di::Result<bool> operator()(di::PathView path) const;
    };
}

constexpr inline auto is_empty = detail::IsEmptyFunction {};
}
