#pragma once

#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>

namespace di::parser {
namespace detail {
    struct CreateParserInPlaceFunction {
        template<typename T, concepts::ParserContext Context>
        requires(concepts::TagInvocable<CreateParserInPlaceFunction, InPlaceType<T>, Context&>)
        constexpr concepts::Parser<Context> auto operator()(InPlaceType<T>, Context& context) const {
            return function::tag_invoke(*this, in_place_type<T>, context);
        }
    };
}

constexpr inline auto create_parser_in_place = detail::CreateParserInPlaceFunction {};
}

namespace di::concepts {
template<typename T, typename Context>
concept Parsable = requires(Context& context) { parser::create_parser_in_place(in_place_type<T>, context); };
}

namespace di::parser {
namespace detail {
    template<typename T>
    struct CreateParserFunction {
        template<concepts::ParserContext Context>
        requires(concepts::Parsable<T, Context>)
        constexpr auto operator()(Context& context) const {
            return create_parser_in_place(in_place_type<T>, context);
        }
    };
}

template<typename T>
constexpr inline auto create_parser = detail::CreateParserFunction<T> {};
}
