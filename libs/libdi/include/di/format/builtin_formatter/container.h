#pragma once

#include <di/concepts/remove_cvref_same_as.h>
#include <di/container/concepts/input_container.h>
#include <di/container/meta/container_reference.h>
#include <di/container/string/constant_string.h>
#include <di/format/concepts/formattable.h>
#include <di/format/formatter.h>
#include <di/format/make_constexpr_format_args.h>
#include <di/format/vpresent_encoded_context.h>

namespace di::format {
template<concepts::InputContainer Con>
requires(!concepts::detail::ConstantString<Con> && concepts::Formattable<meta::ContainerReference<Con>>)
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<Con>, concepts::FormatParseContext auto&) {
    return [](concepts::FormatContext auto& context, concepts::RemoveCVRefSameAs<Con> auto&& container) {
        context.output('{');
        context.output(' ');
        bool first = true;
        for (auto&& value : container) {
            if (!first) {
                context.output(',');
                context.output(' ');
            }
            first = false;
            vpresent_encoded_context<meta::Encoding<decltype(context)>>("{}"_sv, make_constexpr_format_args(value), context);
        }
        context.output(' ');
        context.output('}');
    };
}
}