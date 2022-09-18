#pragma once

#include <di/container/string/string.h>
#include <di/format/concepts/formattable.h>
#include <di/format/present.h>

namespace di::format {
namespace detail {
    struct ToStringFunction {
        constexpr auto operator()(concepts::Formattable auto&& value) const { return present("{}"_sv, value); }
    };
}

constexpr inline auto to_string = detail::ToStringFunction {};
}
