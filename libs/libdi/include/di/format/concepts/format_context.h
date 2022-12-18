#pragma once

#include <di/concepts/maybe_fallible.h>
#include <di/container/string/encoding.h>
#include <di/container/string/utf8_encoding.h>

namespace di::concepts {
template<typename T>
concept FormatContext = requires { typename meta::Encoding<T>; } && requires(T& context, char ascii_code_point) {
                                                                        {
                                                                            context.output(ascii_code_point)
                                                                            } -> concepts::MaybeFallible<void>;
                                                                    };
}

namespace di::format {
struct ContextPlaceholder {
    using Encoding = container::string::Utf8Encoding;
    void output(char);
};
}
