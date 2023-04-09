#pragma once

#include <di/prelude.h>
#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>

namespace iris {
using Encoding = di::container::string::Utf8Encoding;

void log_output_character(c32);
void log_output_byte(di::Byte);

namespace detail {
    struct DebugFormatContext {
        using Encoding = iris::Encoding;

        explicit DebugFormatContext() : m_lock_guard(global_state().debug_output_lock) {}

        void output(c32 value) { log_output_character(value); }

        auto encoding() const { return Encoding {}; }

    private:
        di::ScopedLock<Spinlock> m_lock_guard;
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
