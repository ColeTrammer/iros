#pragma once

#include <di/container/string/encoding.h>
#include <di/container/string/string_impl.h>
#include <di/format/concepts/formattable.h>
#include <di/format/format_string_impl.h>
#include <di/format/make_constexpr_format_args.h>
#include <di/format/vpresent_encoded.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc>
    struct PresentEncodedFunction {
        template<concepts::Formattable... Args>
        constexpr concepts::MaybeFallible<container::string::StringImpl<Enc>> auto operator()(format::FormatStringImpl<Enc, Args...> format,
                                                                                              Args&&... args) const {
            // if consteval {
            return vpresent_encoded<Enc>(format, make_constexpr_format_args(args...));
            // } else {
            // return container::string::StringImpl<Enc> {};
            // }
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto present_encoded = detail::PresentEncodedFunction<Enc> {};
}
