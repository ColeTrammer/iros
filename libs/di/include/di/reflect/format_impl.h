#pragma once

#include <di/format/formatter.h>
#include <di/format/make_constexpr_format_args.h>
#include <di/format/vpresent_encoded_context.h>
#include <di/reflect/reflect.h>
#include <di/types/in_place_type.h>

namespace di::format {
template<concepts::Reflectable T, concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<T>, FormatParseContext<Enc>&) {
    auto do_output = [=](concepts::FormatContext auto& context, T const& value) -> Result<void> {
        context.output('{');
        context.output(' ');
        bool first = true;
        (void) vocab::tuple_for_each(
            [&](auto field) {
                if (!first) {
                    context.output(',');
                    context.output(' ');
                }
                first = false;

                for (auto ch : field.name) {
                    context.output(ch);
                }
                context.output(':');
                context.output(' ');
                (void) vpresent_encoded_context<meta::Encoding<decltype(context)>>(
                    u8"{}"_sv, make_constexpr_format_args(field.get(value)), context);
            },
            reflection::reflect(value));

        context.output(' ');
        context.output('}');
        return {};
    };
    return Result<decltype(do_output)>(util::move(do_output));
}
}
