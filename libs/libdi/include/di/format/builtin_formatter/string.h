#pragma once

#include <di/container/string/constant_string.h>
#include <di/format/formatter.h>

namespace di::format {
template<concepts::detail::ConstantString T, concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<T>, FormatParseContext<Enc>&) {
    auto do_output = [](concepts::FormatContext auto& context, T const& value) -> Result<void> {
        for (auto ch : value.span()) {
            context.output(ch);
        }
        return {};
    };
    return Result<decltype(do_output)>(util::move(do_output));
}
}