#pragma once

#include "di/parser/basic/eof_parser.h"
#include "di/parser/combinator/sequence.h"
#include "di/parser/create_parser.h"
#include "di/parser/into_parser_context.h"

namespace di::parser {
namespace detail {
    template<typename T>
    struct ParseUncheckedFunction {
        template<concepts::IntoParserContext U, typename Context = meta::AsParserContext<U>>
        requires(concepts::Parsable<T, Context>)
        constexpr auto operator()(U&& input) const {
            auto context = into_parser_context(util::forward<U>(input));
            auto parser = sequence(create_parser<T>(context), eof());
            auto result = parser.parse(context);
            DI_ASSERT(result);
            return *util::move(result);
        }
    };
}

template<typename T>
constexpr inline auto parse_unchecked = detail::ParseUncheckedFunction<T> {};
}

namespace di {
using parser::parse_unchecked;
}
