#pragma once

#include <di/format/formatter.h>

namespace di::format {
template<concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<char32_t>, FormatParseContext<Enc>&) {
    return [](concepts::FormatContext auto& context, char32_t value) {
        context.output(value);
    };
}
}