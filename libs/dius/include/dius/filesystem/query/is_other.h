#pragma once

#include <dius/filesystem/query/exists.h>
#include <dius/filesystem/query/is_directory.h>
#include <dius/filesystem/query/is_regular_file.h>
#include <dius/filesystem/query/is_symlink.h>
#include <dius/filesystem/query/status.h>

namespace dius::filesystem {
namespace detail {
    struct IsOtherFunction {
        constexpr bool operator()(FileStatus status) const {
            return exists(status) && !is_regular_file(status) && !is_directory(status) && !is_symlink(status);
        }

        di::Result<bool> operator()(di::PathView path) const { return status(path) % *this; }
    };
}

constexpr inline auto is_other = detail::IsOtherFunction {};
}
