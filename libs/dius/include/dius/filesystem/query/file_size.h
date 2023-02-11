#pragma once

#include <di/prelude.h>

namespace dius::filesystem {
namespace detail {
    struct FileSizeFunction {
        di::Result<umax> operator()(di::PathView path) const;
    };
}

constexpr inline auto file_size = detail::FileSizeFunction {};
}