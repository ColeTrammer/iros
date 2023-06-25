#pragma once

#include <di/container/string/fixed_string.h>
#include <di/container/string/fixed_string_to_utf8_string_view.h>
#include <di/container/string/string_impl.h>
#include <di/container/string/string_view.h>
#include <di/container/string/transparent_encoding.h>
#include <di/container/string/utf8_encoding.h>

namespace di::container {
using String = string::StringImpl<string::Utf8Encoding>;
using TransparentString = string::StringImpl<string::TransparentEncoding>;
}

namespace di {
inline namespace literals {
    inline namespace string_literals {
        template<container::FixedString literal>
        constexpr auto operator""_s() {
            auto view = container::fixed_string_to_utf8_string_view<literal>();
            return view.to_owned();
        }

        constexpr auto operator""_s(c8 const* data, size_t size) {
            auto span = di::Span { data, size };
            DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
            auto view = container::StringView { container::string::encoding::assume_valid, span };
            return view.to_owned();
        }

        constexpr auto operator""_ts(char const* data, size_t size) {
            auto span = di::Span { data, size };
            auto view = container::TransparentStringView { span };
            return view.to_owned();
        }
    }
}
}

namespace di {
using container::String;
using container::TransparentString;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_STRING_LITERALS)
using namespace di::literals::string_literals;
#endif
