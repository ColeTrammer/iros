#pragma once

#include <di/prelude.h>
#include <dius/filesystem/file_status.h>

namespace dius::filesystem {
namespace detail {
    struct StatusFunction {
        di::Result<FileStatus> operator()(di::PathView path) const;
    };
}

constexpr inline auto status = detail::StatusFunction {};
}
