#pragma once

#include <di/concepts/remove_cvref_same_as.h>
#include <di/container/concepts/input_container.h>
#include <di/container/string/constant_string.h>
#include <di/container/string/string_view.h>
#include <di/format/concepts/formattable.h>
#include <di/format/formatter.h>
#include <di/format/make_constexpr_format_args.h>
#include <di/format/vpresent_encoded_context.h>
#include <di/vocab/tuple/prelude.h>

namespace di::format {
template<concepts::Formattable... Types>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<Tuple<Types...>>, concepts::FormatParseContext auto&) {
    return [](concepts::FormatContext auto& context, concepts::DecaySameAs<Tuple<Types...>> auto&& tuple) {
        context.output('(');
        context.output(' ');
        bool first = true;
        tuple_for_each(
            [&](auto&& value) {
                if (!first) {
                    context.output(',');
                    context.output(' ');
                }
                first = false;
                vpresent_encoded_context<meta::Encoding<decltype(context)>>(u8"{}"_sv, make_constexpr_format_args(value), context);
            },
            util::forward<decltype(tuple)>(tuple));
        context.output(' ');
        context.output(')');
    };
}
}