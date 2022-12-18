#pragma once

#include <di/assert/prelude.h>
#include <di/container/string/string_view_impl.h>
#include <di/container/string/transparent_encoding.h>
#include <di/container/string/utf8_encoding.h>
#include <di/vocab/span/prelude.h>

namespace di::container {
using StringView = string::StringViewImpl<string::Utf8Encoding>;
using TransparentStringView = string::StringViewImpl<string::TransparentEncoding>;
}

namespace di {
inline namespace literals {
    inline namespace string_literals {
        constexpr auto operator""_sv(char8_t const* data, size_t size) {
            auto span = di::Span { data, size };
            DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
            return container::StringView { container::string::encoding::assume_valid, span };
        }
    }
}
}
