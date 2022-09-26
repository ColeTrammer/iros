#pragma once

#include <di/container/string/constant_string.h>
#include <di/format/formatter.h>

namespace di::format {
template<concepts::detail::ConstantString T>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<T>, concepts::FormatParseContext auto&) {
    return [](concepts::FormatContext auto& context, T const& value) {
        for (auto ch : value.span()) {
            context.output(ch);
        }
    };
}
}