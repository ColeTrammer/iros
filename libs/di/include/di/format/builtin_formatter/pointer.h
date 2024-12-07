#pragma once

#include "di/format/builtin_formatter/base.h"
#include "di/format/formatter.h"
#include "di/meta/language.h"
#include "di/types/prelude.h"

namespace di::format {
template<typename T, concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<T*>, FormatParseContext<Enc>& parse_context) {
    return parse<detail::PointerFormat>(parse_context.current_format_string()) % [](detail::PointerFormat format) {
        return [=](concepts::FormatContext auto& context, T* value) -> Result<void> {
            auto width = format.width.transform(&detail::Width::value);
            return detail::present_integer_to<Enc>(
                context, format.fill_and_align, detail::Sign::Minus, detail::HashTag::Yes, detail::Zero::No, width,
                detail::IntegerType::HexLower, false, util::bit_cast<uintptr_t>(value));
        };
    };
}

template<concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<nullptr_t>,
                          FormatParseContext<Enc>& parse_context) {
    return parse<detail::PointerFormat>(parse_context.current_format_string()) % [](detail::PointerFormat format) {
        return [=](concepts::FormatContext auto& context, nullptr_t) -> Result<void> {
            auto width = format.width.transform(&detail::Width::value);
            return detail::present_integer_to<Enc>(context, format.fill_and_align, detail::Sign::Minus,
                                                   detail::HashTag::Yes, detail::Zero::No, width,
                                                   detail::IntegerType::HexLower, false, 0U);
        };
    };
}
}
