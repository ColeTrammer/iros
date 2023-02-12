#pragma once

#include <di/format/builtin_formatter/base.h>
#include <di/format/formatter.h>

namespace di::format {
template<concepts::Encoding Enc, concepts::OneOf<char, c32> T>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<T>, FormatParseContext<Enc>& parse_context,
                          bool debug) {
    return parse<detail::CharacterFormat>(parse_context.current_format_string()) % [debug](
                                                                                       detail::CharacterFormat format) {
        return [=](concepts::FormatContext auto& context, T value) -> Result<void> {
            auto type = format.type.value_or(debug ? detail::CharacterType::Debug : detail::CharacterType::Character);
            auto do_debug = false;
            if (type == detail::CharacterType::Debug) {
                do_debug = true;
                type = detail::CharacterType::Character;
            }
            auto width = format.width.transform(&detail::Width::value);
            return detail::present_integer_to<Enc>(context, format.fill_and_align, format.sign, format.hash_tag,
                                                   format.zero, width, static_cast<detail::IntegerType>(type), do_debug,
                                                   value);
        };
    };
}
}