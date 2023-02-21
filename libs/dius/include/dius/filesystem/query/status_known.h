#pragma once

#include <di/prelude.h>
#include <dius/filesystem/file_status.h>

namespace dius::filesystem {
namespace detail {
    struct StatusKnownFunction {
        constexpr bool operator()(FileStatus status) const { return status.type() != FileType::None; }
    };
}

constexpr inline auto status_known = detail::StatusKnownFunction {};
}
