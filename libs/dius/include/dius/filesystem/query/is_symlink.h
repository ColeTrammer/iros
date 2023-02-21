#pragma once

#include <dius/filesystem/query/symlink_status.h>

namespace dius::filesystem {
namespace detail {
    struct IsSymlinkFunction {
        constexpr bool operator()(FileStatus status) const { return status.type() == FileType::Symlink; }

        di::Result<bool> operator()(di::PathView path) const { return symlink_status(path) % *this; }
    };
}

constexpr inline auto is_symlink = detail::IsSymlinkFunction {};
}
