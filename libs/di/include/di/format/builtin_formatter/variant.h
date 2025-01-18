#pragma once

#include "di/container/concepts/input_container.h"
#include "di/container/string/constant_string.h"
#include "di/container/string/string_view.h"
#include "di/format/concepts/formattable.h"
#include "di/format/formatter.h"
#include "di/format/make_format_args.h"
#include "di/format/vpresent_encoded_context.h"
#include "di/meta/util.h"
#include "di/vocab/variant/prelude.h"

namespace di::format {
template<concepts::Formattable... Types, concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<Variant<Types...>>, FormatParseContext<Enc>&) {
    auto do_output = [](concepts::FormatContext auto& context,
                        concepts::DecaySameAs<Variant<Types...>> auto&& variant) -> Result<void> {
        return di::visit(
            [&](auto&& value) -> Result<void> {
                return vpresent_encoded_context<meta::Encoding<decltype(context)>>(
                    u8"{}"_sv, format::make_format_args<decltype(context)>(value), context);
            },
            variant);
    };
    return Result<decltype(do_output)>(util::move(do_output));
}
}
