#pragma once

#include <dius/filesystem/query/status.h>
#include <dius/filesystem/query/status_known.h>

namespace dius::filesystem {
namespace detail {
    struct ExistsFunction {
        constexpr auto operator()(FileStatus status) const -> bool {
            return status_known(status) && status.type() != FileType::NotFound;
        }

        auto operator()(di::PathView path) const -> di::Result<bool> { return status(path) % *this; }
    };
}

constexpr inline auto exists = detail::ExistsFunction {};
}
