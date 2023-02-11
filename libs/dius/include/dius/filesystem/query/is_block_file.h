#pragma once

#include <dius/filesystem/query/status.h>

namespace dius::filesystem {
namespace detail {
    struct IsBlockFileFunction {
        constexpr bool operator()(FileStatus status) const { return status.type() == FileType::Block; }

        di::Result<bool> operator()(di::PathView path) const { return status(path) % *this; }
    };
}

constexpr inline auto is_block_file = detail::IsBlockFileFunction {};
}