#pragma once

#include <di/container/string/string_view.h>
#include <di/format/builtin_formatter/base.h>
#include <di/format/formatter.h>

namespace di::format {
template<concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<bool>, FormatParseContext<Enc>& parse_context) {
    return parse<detail::BoolFormat>(parse_context.current_format_string()) % [](detail::BoolFormat format) {
        return [=](concepts::FormatContext auto& context, bool value) -> Result<void> {
            auto type = format.type;
            auto width = format.width.transform(&detail::Width::value);
            if (type == detail::BoolType::String) {
                using namespace string_literals;
                return detail::present_string_view_to(context, format.fill_and_align, width, nullopt, false,
                                                      value ? "true"_sv : "false"_sv);
            }
            return detail::present_integer_to<Enc>(context, format.fill_and_align, format.sign, format.hash_tag,
                                                   format.zero, width, static_cast<detail::IntegerType>(type), false,
                                                   static_cast<i8>(value));
           
        };
    };
}
}
