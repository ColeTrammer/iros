#pragma once

#include <di/format/formatter.h>

namespace di::format {
template<concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<char32_t>, FormatParseContext<Enc>&) {
    auto do_output = [](concepts::FormatContext auto& context, char32_t value) -> Result<void> {
        context.output(value);
        return {};
    };
    return Result<decltype(do_output)>(util::move(do_output));
}
}