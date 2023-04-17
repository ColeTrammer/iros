#pragma once

#include <di/container/path/prelude.h>
#include <di/vocab/error/prelude.h>

namespace dius::filesystem {
namespace detail {
    struct FileSizeFunction {
        di::Result<umax> operator()(di::PathView path) const;
    };
}

constexpr inline auto file_size = detail::FileSizeFunction {};
}
