#pragma once

#include <di/container/concepts/input_container.h>
#include <di/container/meta/container_reference.h>
#include <di/container/string/constant_string.h>
#include <di/container/string/string_view.h>
#include <di/format/concepts/formattable.h>
#include <di/format/formatter.h>
#include <di/format/make_constexpr_format_args.h>
#include <di/format/vpresent_encoded_context.h>
#include <di/meta/util.h>

namespace di::format {
template<concepts::InputContainer Con, concepts::Encoding Enc>
requires(!concepts::detail::ConstantString<Con> && concepts::Formattable<meta::ContainerReference<Con>>)
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<Con>, FormatParseContext<Enc>&) {
    auto do_output = [](concepts::FormatContext auto& context,
                        concepts::RemoveCVRefSameAs<Con> auto&& container) -> Result<void> {
        context.output('{');
        context.output(' ');
        bool first = true;
        for (auto&& value : container) {
            if (!first) {
                context.output(',');
                context.output(' ');
            }
            first = false;
            DI_TRY(vpresent_encoded_context<meta::Encoding<decltype(context)>>(
                u8"{}"_sv, make_constexpr_format_args(value), context));
        }
        context.output(' ');
        context.output('}');
        return {};
    };
    return Result<decltype(do_output)>(util::move(do_output));
}
}
