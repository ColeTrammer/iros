#pragma once

#include <di/format/formatter.h>
#include <di/types/prelude.h>

namespace di::format {
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<Void>, concepts::FormatParseContext auto&) {
    return [](concepts::FormatContext auto& context, Void) {
        context.output('v');
        context.output('o');
        context.output('i');
        context.output('d');
    };
}
}