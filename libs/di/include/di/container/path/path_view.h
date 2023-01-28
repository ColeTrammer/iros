#pragma once

#include <di/container/path/path_view_impl.h>
#include <di/container/string/string_view.h>
#include <di/container/string/transparent_encoding.h>
#include <di/container/string/utf8_encoding.h>

namespace di {
namespace container {
    using PathView = PathViewImpl<string::TransparentEncoding>;
    using Utf8PathView = PathViewImpl<string::Utf8Encoding>;
}

inline namespace literals {
    inline namespace path_view_literals {
        template<container::FixedString literal>
        constexpr auto operator""_u8pv() {
            return function::unpack<meta::MakeIndexSequence<literal.size()>>(
                []<size_t... indices>(meta::IndexSequence<indices...>) {
                    if constexpr (literal.size() == 0) {
                        return container::Utf8PathView { container::StringView {} };
                    } else {
                        auto span = Span { as_u8_buffer<literal, indices...> };
                        DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
                        return container::Utf8PathView { container::StringView {
                            container::string::encoding::assume_valid, span } };
                    }
                });
        }

        constexpr auto operator"" _u8pv(c8 const* data, size_t size) {
            auto span = di::Span { data, size };
            DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
            return container::Utf8PathView { container::StringView { container::string::encoding::assume_valid,
                                                                     span } };
        }

        constexpr auto operator"" _pv(char const* data, size_t size) {
            auto span = di::Span { data, size };
            return container::PathView { container::TransparentStringView { span } };
        }
    }
}
}