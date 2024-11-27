#pragma once

#include <di/container/path/prelude.h>
#include <di/vocab/error/prelude.h>
#include <dius/filesystem/file_status.h>

namespace dius::filesystem {
namespace detail {
    struct StatusFunction {
        auto operator()(di::PathView path) const -> di::Result<FileStatus>;
    };
}

constexpr inline auto status = detail::StatusFunction {};
}
