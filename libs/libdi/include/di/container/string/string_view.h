#pragma once

#include <di/assert/prelude.h>
#include <di/container/string/string_view_impl.h>
#include <di/container/string/transparent_encoding.h>
#include <di/vocab/span/prelude.h>

namespace di::container {
using StringView = string::StringViewImpl<string::TransparentEncoding>;
}

namespace di {
inline namespace literals {
    inline namespace string_literals {
        constexpr container::StringView operator""_sv(char const* data, size_t size) {
            auto span = di::Span { data, size };
            DI_ASSERT(container::string::encoding::validate(container::string::TransparentEncoding(), span));
            return container::StringView { container::string::encoding::assume_valid, span };
        }
    }
}
}
