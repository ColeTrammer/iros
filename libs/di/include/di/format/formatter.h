#pragma once

#include <di/concepts/maybe_fallible.h>
#include <di/format/concepts/format_context.h>
#include <di/format/format_parse_context.h>
#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>

namespace di::format {
struct FormatterInPlaceFunction {
    template<typename T, concepts::Encoding Enc>
    requires(concepts::TagInvocable<FormatterInPlaceFunction, InPlaceType<T>, FormatParseContext<Enc>&, bool> ||
             concepts::TagInvocable<FormatterInPlaceFunction, InPlaceType<T>, FormatParseContext<Enc>&>)
    constexpr auto operator()(InPlaceType<T>, FormatParseContext<Enc>& context, bool debug = false) const {
        if constexpr (concepts::TagInvocable<FormatterInPlaceFunction, InPlaceType<T>, FormatParseContext<Enc>&,
                                             bool>) {
            return function::tag_invoke(*this, in_place_type<T>, context, debug);
        } else {
            return function::tag_invoke(*this, in_place_type<T>, context);
        }
    }
};

constexpr inline auto formatter_in_place = FormatterInPlaceFunction {};

template<typename T, concepts::Encoding Enc>
constexpr auto formatter(FormatParseContext<Enc>& parse_context, bool debug = false)
requires(requires { formatter_in_place(in_place_type<meta::RemoveCVRef<T>>, parse_context); })
{
    return formatter_in_place(in_place_type<meta::RemoveCVRef<T>>, parse_context, debug);
}
}
