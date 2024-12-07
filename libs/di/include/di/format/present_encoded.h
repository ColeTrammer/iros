#pragma once

#include "di/container/string/encoding.h"
#include "di/container/string/string_impl.h"
#include "di/format/concepts/formattable.h"
#include "di/format/format_string_impl.h"
#include "di/format/make_format_args.h"
#include "di/format/vpresent_encoded.h"

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc>
    struct PresentEncodedFunction {
        template<concepts::Formattable... Args>
        constexpr auto operator()(format::FormatStringImpl<Enc, Args...> format, Args&&... args) const {
            return vpresent_encoded<Enc>(format, format::make_format_args<FormatContext<Enc>>(args...));
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto present_encoded = detail::PresentEncodedFunction<Enc> {};
}
