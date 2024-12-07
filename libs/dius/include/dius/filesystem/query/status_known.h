#pragma once

#include "di/container/path/prelude.h"
#include "di/vocab/error/prelude.h"
#include "dius/filesystem/file_status.h"

namespace dius::filesystem {
namespace detail {
    struct StatusKnownFunction {
        constexpr auto operator()(FileStatus status) const -> bool { return status.type() != FileType::None; }
    };
}

constexpr inline auto status_known = detail::StatusKnownFunction {};
}
