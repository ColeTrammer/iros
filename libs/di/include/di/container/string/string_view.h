#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/string/fixed_string.h>
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
        template<container::FixedString literal, size_t... indices>
        constexpr inline c8 const as_u8_buffer[sizeof...(indices)] = { static_cast<char>(literal.data()[indices])... };

        template<container::FixedString literal>
        constexpr auto operator""_sv() {
            return function::unpack<meta::MakeIndexSequence<literal.size()>>(
                []<size_t... indices>(meta::IndexSequence<indices...>) {
                    if constexpr (literal.size() == 0) {
                        return container::StringView {};
                    } else {
                        auto span = Span { as_u8_buffer<literal, indices...> };
                        DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
                        return container::StringView { container::string::encoding::assume_valid, span };
                    }
                });
        }

        constexpr auto operator""_sv(c8 const* data, size_t size) {
            auto span = di::Span { data, size };
            DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
            return container::StringView { container::string::encoding::assume_valid, span };
        }

        constexpr auto operator""_tsv(char const* data, size_t size) {
            auto span = di::Span { data, size };
            return container::TransparentStringView { span };
        }
    }
}
}
