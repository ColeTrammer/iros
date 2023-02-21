#pragma once

#include <dius/filesystem/query/status.h>
#include <dius/filesystem/query/status_known.h>

namespace dius::filesystem {
namespace detail {
    struct ExistsFunction {
        constexpr bool operator()(FileStatus status) const {
            return status_known(status) && status.type() != FileType::NotFound;
        }

        di::Result<bool> operator()(di::PathView path) const { return status(path) % *this; }
    };
}

constexpr inline auto exists = detail::ExistsFunction {};
}
