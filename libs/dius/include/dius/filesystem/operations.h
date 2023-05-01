#pragma once

#include <di/container/path/prelude.h>

namespace dius::filesystem {
namespace detail {
    struct CreateRegularFileFunction {
        di::Result<bool> operator()(di::PathView path) const;
    };
}

constexpr inline auto create_regular_file = detail::CreateRegularFileFunction {};

namespace detail {
    struct CreateDirectoryFunction {
        di::Result<bool> operator()(di::PathView path) const;
    };
}

constexpr inline auto create_directory = detail::CreateDirectoryFunction {};
}
