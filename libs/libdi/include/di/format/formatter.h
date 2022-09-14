#pragma once

#include <di/concepts/maybe_fallible.h>
#include <di/format/format_context.h>
#include <di/format/format_parse_context.h>
#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>
#include <di/util/prelude.h>

namespace di::format {
namespace detail {
    struct FormatterInPlaceFunction {
        template<typename T, concepts::FormatParseContext ParseContext>
        requires(concepts::TagInvocable<FormatterInPlaceFunction, InPlaceType<T>, ParseContext&>)
        constexpr auto operator()(InPlaceType<T>, ParseContext& context) const {
            return function::tag_invoke(in_place_type<T>, context);
        }
    };
}

constexpr inline auto formatter_in_place = detail::FormatterInPlaceFunction {};

template<typename T>
constexpr auto formatter(concepts::FormatParseContext auto& parse_context) {
    return formatter_in_place(in_place_type<T>, parse_context);
}
}
