#pragma once

#include <di/concepts/decay_constructible.h>
#include <di/function/into_void.h>
#include <di/parser/combinator/transform.h>

namespace di::parser {
namespace detail {
    struct IgnoreFunction {
        template<concepts::DecayConstructible Parser>
        constexpr auto operator()(Parser&& parser) const {
            return util::forward<Parser>(parser) % function::into_void;
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