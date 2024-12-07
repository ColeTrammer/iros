#pragma once

#include "di/assert/assert_bool.h"
#include "di/container/string/fixed_string.h"
#include "di/container/string/fixed_string_to_utf8_string_view.h"
#include "di/container/string/string_view_impl.h"
#include "di/container/string/transparent_encoding.h"
#include "di/container/string/utf8_encoding.h"
#include "di/vocab/span/prelude.h"

namespace di::container {
using StringView = string::StringViewImpl<string::Utf8Encoding>;
using TransparentStringView = string::StringViewImpl<string::TransparentEncoding>;
}

namespace di {
inline namespace literals {
    inline namespace string_view_literals {
        template<container::FixedString literal>
        consteval auto operator""_sv() {
            return container::fixed_string_to_utf8_string_view<literal>();
        }

        consteval auto operator""_sv(c8 const* data, size_t size) {
            auto span = di::Span { data, size };
            DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
            return container::StringView { container::string::encoding::assume_valid, span };
        }

        consteval auto operator""_tsv(char const* data, size_t size) {
            auto span = di::Span { data, size };
            return container::TransparentStringView { span };
        }
    }
}
}

namespace di {
using container::StringView;
using container::TransparentStringView;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_STRING_LITERALS)
using namespace di::literals::string_view_literals;
#endif
