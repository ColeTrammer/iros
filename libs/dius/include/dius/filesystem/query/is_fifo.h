#pragma once

#include <dius/filesystem/query/status.h>

namespace dius::filesystem {
namespace detail {
    struct IsFifoFunction {
        constexpr bool operator()(FileStatus status) const { return status.type() == FileType::Directory; }

        di::Result<bool> operator()(di::PathView path) const { return status(path) % *this; }
    };
}

constexpr inline auto is_fifo = detail::IsFifoFunction {};
}