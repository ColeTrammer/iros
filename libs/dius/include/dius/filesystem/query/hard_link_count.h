#pragma once

#include <di/container/path/prelude.h>
#include <di/vocab/error/prelude.h>

namespace dius::filesystem {
namespace detail {
    struct HardLinkCountFunction {
        di::Result<umax> operator()(di::PathView path) const;
    };
}

constexpr inline auto hard_link_count = detail::HardLinkCountFunction {};
}
