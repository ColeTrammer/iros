#pragma once

#include <dius/filesystem/query/symlink_status.h>

namespace dius::filesystem {
namespace detail {
    struct IsSymlinkFunction {
        constexpr auto operator()(FileStatus status) const -> bool { return status.type() == FileType::Symlink; }

        auto operator()(di::PathView path) const -> di::Result<bool> { return symlink_status(path) % *this; }
    };
}

constexpr inline auto is_symlink = detail::IsSymlinkFunction {};
}
