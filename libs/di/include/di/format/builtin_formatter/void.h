#pragma once

#include <di/format/formatter.h>
#include <di/types/prelude.h>

namespace di::format {
template<concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<Void>, FormatParseContext<Enc>&) {
    auto do_output = [](concepts::FormatContext auto& context, Void) -> Result<void> {
        context.output('v');
        context.output('o');
        context.output('i');
        context.output('d');
        return {};
    };
    return Result<decltype(do_output)>(util::move(do_output));
}
}
