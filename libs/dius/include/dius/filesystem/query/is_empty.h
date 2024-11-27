#pragma once

#include <di/container/path/prelude.h>
#include <di/vocab/error/prelude.h>

namespace dius::filesystem {
namespace detail {
    struct IsEmptyFunction {
        auto operator()(di::PathView path) const -> di::Result<bool>;
    };
}

constexpr inline auto is_empty = detail::IsEmptyFunction {};
}
