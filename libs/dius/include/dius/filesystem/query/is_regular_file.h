#pragma once

#include <dius/filesystem/query/status.h>

namespace dius::filesystem {
namespace detail {
    struct IsRegularFileFunction {
        constexpr bool operator()(FileStatus status) const { return status.type() == FileType::Regular; }

        di::Result<bool> operator()(di::PathView path) const { return status(path) % *this; }
    };
}

constexpr inline auto is_regular_file = detail::IsRegularFileFunction {};
}
