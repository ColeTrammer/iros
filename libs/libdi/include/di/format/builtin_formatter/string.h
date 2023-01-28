#pragma once

#include <di/container/string/constant_string.h>
#include <di/format/builtin_formatter/base.h>
#include <di/format/formatter.h>

namespace di::format {
template<concepts::detail::ConstantString T, concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<T>, FormatParseContext<Enc>& parse_context) {
    return parse<detail::StringFormat>(parse_context.current_format_string()) % [](detail::StringFormat format) {
        return [=](concepts::FormatContext auto& context, T const& value) -> Result<void> {
            auto width = format.width.transform(&detail::Width::value);
            auto precision = format.precision.transform(&detail::Precision::value);
            auto debug = format.type == detail::StringType::Debug;
            return detail::present_string_view_to(context, format.fill_and_align, width, precision, debug,
                                                  value.view());
        };
    };
}
}