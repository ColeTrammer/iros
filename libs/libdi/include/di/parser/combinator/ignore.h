#pragma once

#include <di/parser/combinator/transform.h>

namespace di::parser {
namespace detail {
    struct IgnoreFunction {
        template<concepts::DecayConstructible Parser>
        constexpr auto operator()(Parser&& parser) const {
            return util::forward<Parser>(parser) % [](auto&&...) {};
        }
    };
}

constexpr inline auto ignore = detail::IgnoreFunction {};

template<concepts::DecayConstructible Parser>
requires(concepts::DerivedFrom<Parser, ParserBase<Parser>>)
constexpr auto operator~(Parser&& parser) {
    return ignore(util::forward<Parser>(parser));
}
}