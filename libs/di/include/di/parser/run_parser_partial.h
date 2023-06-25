#pragma once

#include <di/function/curry.h>
#include <di/parser/concepts/parser.h>
#include <di/parser/into_parser_context.h>

namespace di::parser {
namespace detail {
    struct RunParserPartialFunction {
        template<concepts::IntoParserContext U, typename Context = meta::AsParserContext<U>,
                 concepts::Parser<Context> Parser>
        constexpr auto operator()(Parser parser, U&& input) const {
            auto context = into_parser_context(util::forward<U>(input));
            return util::forward<Parser>(parser).parse(context);
        }
    };
}

constexpr inline auto run_parser_partial = function::curry(detail::RunParserPartialFunction {}, meta::c_<2zu>);
}

namespace di {
using parser::run_parser_partial;
}
