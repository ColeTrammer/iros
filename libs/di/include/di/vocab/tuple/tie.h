#pragma once

#include "di/vocab/tuple/tuple.h"

namespace di::vocab {
namespace detail {
    struct TieFunction {
        template<typename... Types>
        constexpr auto operator()(Types&... references) const {
            return Tuple<Types&...>(references...);
        }
    };
}

constexpr inline auto tie = detail::TieFunction {};
}

namespace di {
using vocab::tie;
}
