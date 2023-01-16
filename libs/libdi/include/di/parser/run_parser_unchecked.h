#pragma once

#include <di/parser/run_parser.h>

namespace di::parser {
namespace detail {
    struct RunParserUncheckedFunction {
        template<concepts::IntoParserContext U, typename Context = meta::AsParserContext<U>, concepts::Parser<Context> Parser>
        constexpr decltype(auto) operator()(Parser parser, U&& input) const {
            return *run_parser(util::move(parser), util::forward<U>(input));
        }
    };
}

constexpr inline auto run_parser_unchecked = function::curry(detail::RunParserUncheckedFunction {}, meta::size_constant<2>);
}