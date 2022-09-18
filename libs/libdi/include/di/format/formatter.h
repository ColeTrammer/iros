#pragma once

#include <di/concepts/maybe_fallible.h>
#include <di/format/concepts/format_context.h>
#include <di/format/concepts/format_parse_context.h>
#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>

namespace di::format {
namespace detail {
    struct FormatterInPlaceFunction {
        template<typename T, concepts::FormatParseContext ParseContext>
        requires(concepts::SameAs<T, int> || concepts::TagInvocable<FormatterInPlaceFunction, InPlaceType<T>, ParseContext&>)
        constexpr auto operator()(InPlaceType<T>, ParseContext& context) const {
            if constexpr (concepts::SameAs<T, int>) {
                return [](concepts::FormatParseContext auto& parse_context, int) {
                    parse_context.output('4');
                    parse_context.output('2');
                };
            } else {
                return function::tag_invoke(*this, in_place_type<T>, context);
            }
        }
    };
}

constexpr inline auto formatter_in_place = detail::FormatterInPlaceFunction {};

template<typename T>
constexpr auto formatter(concepts::FormatParseContext auto& parse_context)
requires(requires { formatter_in_place(in_place_type<meta::RemoveCVRef<T>>, parse_context); })
{
    return formatter_in_place(in_place_type<meta::RemoveCVRef<T>>, parse_context);
}
}
