#pragma once

#include <di/container/string/encoding.h>
#include <di/container/string/string_impl.h>
#include <di/format/format_string_impl.h>
#include <di/format/formattable.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc, concepts::Formattable... Args>
    struct PresentEncodedFunction {
        constexpr concepts::MaybeFallible<container::string::StringImpl<Enc>> auto operator()(FormatStringImpl<Enc, Args...>,
                                                                                              Args&&...) const {
            return container::string::StringImpl<Enc> {};
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto present_encoded = detail::PresentEncodedFunction<Enc> {};
}
