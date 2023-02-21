#pragma once

#include <di/format/prelude.h>
#include <di/io/writer_format_context.h>

namespace di::io {
namespace detail {
    template<concepts::Encoding Enc>
    struct WriterPrintFunction {
        template<Impl<Writer> Writer, typename... Args, typename Sv = container::string::StringViewImpl<Enc>>
        constexpr void operator()(Writer& writer, Sv format_string, Args&&... args) const {
            auto context = WriterFormatContext<Writer, Enc>(writer, format_string.encoding());
            if consteval {
                (void) format::vpresent_encoded_context<Enc>(format_string, format::make_constexpr_format_args(args...),
                                                             context);
            } else {
                (void) format::vpresent_encoded_context<Enc>(
                    format_string, format::make_format_args<WriterFormatContext<Writer, Enc>>(args...), context);
            }
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto writer_print = detail::WriterPrintFunction<Enc> {};
}
