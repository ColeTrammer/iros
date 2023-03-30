#pragma once

#include <di/concepts/maybe_fallible.h>
#include <di/container/string/encoding.h>
#include <di/container/string/utf8_encoding.h>

namespace di::concepts {
template<typename T>
concept FormatContext = requires { typename meta::Encoding<T>; } && requires(T& context, char ascii_code_point) {
    { context.output(ascii_code_point) } -> SameAs<void>;
    { util::as_const(context).encoding() } -> SameAs<meta::Encoding<T>>;
};
}

namespace di::format {
struct ContextPlaceholder {
    using Encoding = container::string::Utf8Encoding;
    void output(char);
    Encoding encoding() const;
};
}
