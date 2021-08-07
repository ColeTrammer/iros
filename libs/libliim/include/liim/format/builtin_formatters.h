#pragma once

#include <liim/format/format_args.h>
#include <liim/format/format_context.h>

namespace LIIM::Format {
template<>
struct Formatter<StringView> {
    void format(const StringView& value, FormatContext& context) { context.put(value); }
};
}
