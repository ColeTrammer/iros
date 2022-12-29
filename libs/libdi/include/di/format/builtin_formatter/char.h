#pragma once

#include <di/format/formatter.h>

namespace di::format {
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<char32_t>, concepts::FormatParseContext auto&) {
    return [](concepts::FormatContext auto& context, char32_t value) {
        context.output(value);
    };
}
}