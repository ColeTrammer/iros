#pragma once

#include <di/format/prelude.h>
#include <di/util/prelude.h>
#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>

namespace iris {
using Encoding = di::container::string::Utf8Encoding;

namespace detail {
    struct DebugFormatContext;
}

void log_output_character(c32);
void log_output_byte(di::Byte);
void log_prologue(detail::DebugFormatContext&, di::SourceLocation = di::SourceLocation::current());

namespace detail {
    struct DebugFormatContext {
        using Encoding = iris::Encoding;
        using SupportsStyle = void;

        explicit DebugFormatContext() : m_lock_guard(global_state().debug_output_lock) {}

        void output(c32 value) { log_output_character(value); }

        auto encoding() const { return Encoding {}; }

        di::Result<void> with_style(di::format::Style style, di::concepts::InvocableTo<di::Result<void>> auto inner) {
            auto [before, after] = style.render_to_ansi_escapes<Encoding>();
            for (auto code_point : before) {
                output(code_point);
            }
            TRY(inner());
            for (auto code_point : after) {
                output(code_point);
            }
            return {};
        }

    private:
        di::ScopedLock<Spinlock> m_lock_guard;
    };
}

template<typename... Args>
void print(di::format::FormatStringWithLocationImpl<Encoding, Args...> format, Args&&... args) {
    auto context = detail::DebugFormatContext {};
    (void) di::format::vpresent_encoded_context<Encoding>(
        format, di::format::make_format_args<detail::DebugFormatContext>(args...), context);
}

template<typename... Args>
void println(di::format::FormatStringWithLocationImpl<Encoding, Args...> format, Args&&... args) {
    auto context = detail::DebugFormatContext {};
    log_prologue(context, format.location());
    (void) di::format::vpresent_encoded_context<Encoding>(
        format, di::format::make_format_args<detail::DebugFormatContext>(args...), context);
    context.output('\n');
}
}
