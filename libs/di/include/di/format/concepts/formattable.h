#pragma once

#include "di/format/concepts/format_context.h"
#include "di/format/format_parse_context.h"
#include "di/format/formatter.h"

namespace di::concepts {
template<typename T>
concept Formattable = requires(format::ContextPlaceholder& context,
                               format::FormatParseContext<container::string::Utf8Encoding> parse_context, T&& value) {
    { (*format::formatter<T>(parse_context))(context, util::forward<T>(value)) } -> SameAs<Result<void>>;
};
}

namespace di {
using concepts::Formattable;
}
