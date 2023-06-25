#pragma once

#include <di/parser/basic/eof_parser.h>
#include <di/parser/combinator/sequence.h>
#include <di/parser/create_parser.h>
#include <di/parser/into_parser_context.h>

namespace di::parser {
namespace detail {
    template<typename T>
    struct ParseFunction {
        template<concepts::IntoParserContext U, typename Context = meta::AsParserContext<U>>
        requires(concepts::Parsable<T, Context>)
        constexpr auto operator()(U&& input) const {
            auto context = into_parser_context(util::forward<U>(input));
            auto parser = sequence(create_parser<T>(context), eof());
            return parser.parse(context);
        }
    };
}

template<typename T>
constexpr inline auto parse = detail::ParseFunction<T> {};
}

namespace di {
using parser::parse;
}
