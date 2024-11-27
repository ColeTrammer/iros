#pragma once

#include <dius/filesystem/query/status.h>

namespace dius::filesystem {
namespace detail {
    struct IsSocketFunction {
        constexpr auto operator()(FileStatus status) const -> bool { return status.type() == FileType::Socket; }

        auto operator()(di::PathView path) const -> di::Result<bool> { return status(path) % *this; }
    };
}

constexpr inline auto is_socket = detail::IsSocketFunction {};
}
