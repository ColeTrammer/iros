#pragma once

#include <di/prelude.h>
#include <iris/core/interrupt_disabler.h>

namespace iris {
using Encoding = di::container::string::Utf8Encoding;

void log_output_character(c32);
void log_output_byte(di::Byte);

namespace detail {
    struct DebugFormatContext {
        using Encoding = iris::Encoding;

        void output(c32 value) { log_output_character(value); }

        auto encoding() const { return Encoding {}; }

    private:
        InterruptDisabler m_interrupt_disabler;
    };
}

template<typename... Args>
void print(di::format::FormatStringImpl<Encoding, Args...> format, Args&&... args) {
    auto context = detail::DebugFormatContext {};
    (void) di::format::vpresent_encoded_context<Encoding>(
        format, di::format::make_format_args<detail::DebugFormatContext>(args...), context);
}

template<typename... Args>
void println(di::format::FormatStringImpl<Encoding, Args...> format, Args&&... args) {
    auto context = detail::DebugFormatContext {};
    (void) di::format::vpresent_encoded_context<Encoding>(
        format, di::format::make_format_args<detail::DebugFormatContext>(args...), context);
    context.output('\n');
}
}
