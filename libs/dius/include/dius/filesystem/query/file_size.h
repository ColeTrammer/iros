#pragma once

#include <di/container/path/prelude.h>
#include <di/vocab/error/prelude.h>

namespace dius::filesystem {
namespace detail {
    struct FileSizeFunction {
        auto operator()(di::PathView path) const -> di::Result<umax>;
    };
}

constexpr inline auto file_size = detail::FileSizeFunction {};
}
