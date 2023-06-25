#pragma once

#include <di/function/curry.h>
#include <di/parser/basic/eof_parser.h>
#include <di/parser/combinator/sequence.h>
#include <di/parser/concepts/parser.h>
#include <di/parser/into_parser_context.h>

namespace di::parser {
namespace detail {
    struct RunParserFunction {
        template<concepts::IntoParserContext U, typename Context = meta::AsParserContext<U>,
                 concepts::Parser<Context> Parser>
        constexpr auto operator()(Parser parser, U&& input) const {
            auto context = into_parser_context(util::forward<U>(input));
            auto with_eof_parser = parser::sequence(util::move(parser), parser::eof());
            return with_eof_parser.parse(context);
        }
    };
}

constexpr inline auto run_parser = detail::RunParserFunction {};
}

namespace di {
using parser::run_parser;
}
