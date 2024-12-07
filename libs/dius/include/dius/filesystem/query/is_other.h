#pragma once

#include "dius/filesystem/query/exists.h"
#include "dius/filesystem/query/is_directory.h"
#include "dius/filesystem/query/is_regular_file.h"
#include "dius/filesystem/query/is_symlink.h"
#include "dius/filesystem/query/status.h"

namespace dius::filesystem {
namespace detail {
    struct IsOtherFunction {
        constexpr auto operator()(FileStatus status) const -> bool {
            return exists(status) && !is_regular_file(status) && !is_directory(status) && !is_symlink(status);
        }

        auto operator()(di::PathView path) const -> di::Result<bool> { return status(path) % *this; }
    };
}

constexpr inline auto is_other = detail::IsOtherFunction {};
}
