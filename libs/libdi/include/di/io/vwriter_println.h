#pragma once

#include <di/format/prelude.h>
#include <di/io/writer_format_context.h>

namespace di::io {
namespace detail {
    template<concepts::Encoding Enc>
    struct VWriterPrintlnFunction {
        template<concepts::Writer Writer, typename... Args, typename Sv = container::string::StringViewImpl<Enc>>
        constexpr void operator()(Writer& writer, Sv format_string, Args&&... args) const {
            auto context = WriterFormatContext<Writer, Enc>(writer, format_string.encoding());
            (void) format::vpresent_encoded_context<Enc>(format_string, format::make_constexpr_format_args(args...), context);
            (void) context.output('\n');
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto vwriter_println = detail::VWriterPrintlnFunction<Enc> {};
}