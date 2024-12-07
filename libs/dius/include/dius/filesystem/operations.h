#pragma once

#include "di/container/path/prelude.h"

namespace dius::filesystem {
namespace detail {
    struct CreateRegularFileFunction {
        auto operator()(di::PathView path) const -> di::Result<bool>;
    };
}

constexpr inline auto create_regular_file = detail::CreateRegularFileFunction {};

namespace detail {
    struct CreateDirectoryFunction {
        auto operator()(di::PathView path) const -> di::Result<bool>;
    };
}

constexpr inline auto create_directory = detail::CreateDirectoryFunction {};
}
