#pragma once

#include <di/format/builtin_formatter/base.h>
#include <di/format/formatter.h>

namespace di::format {
template<concepts::Encoding Enc, concepts::OneOf<char, c32> T>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<T>, FormatParseContext<Enc>& parse_context) {
    return parse<detail::CharacterFormat>(parse_context.current_format_string()) % [](detail::CharacterFormat format) {
        return [=](concepts::FormatContext auto& context, T value) -> Result<void> {
            bool debug = false;
            auto type = format.type;
            if (type == detail::CharacterType::Debug) {
                debug = true;
                type = detail::CharacterType::Character;
            }
            auto width = format.width.transform(&detail::Width::value);
            return detail::present_integer_to<Enc>(context, format.fill_and_align, format.sign, format.hash_tag,
                                                   format.zero, width, static_cast<detail::IntegerType>(type), debug,
                                                   value);
        };
    };
}
}