#pragma once

#include <di/prelude.h>

namespace iris {
using Encoding = di::container::string::TransparentEncoding;

void log_output_character(char);

namespace detail {
    struct DebugFormatContext {
        using Encoding = iris::Encoding;

        void output(char value) { log_output_character(value); }
    };
}

template<typename... Args>
void debug_log(di::format::FormatStringImpl<Encoding, Args...> format, Args&&... args) {
    auto context = detail::DebugFormatContext {};
    return di::format::vpresent_encoded_context<Encoding>(format, di::format::make_constexpr_format_args(args...), context);
}
}
