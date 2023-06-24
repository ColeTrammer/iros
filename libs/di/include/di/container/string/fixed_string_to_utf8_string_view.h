#pragma once

#include <di/container/string/fixed_string.h>
#include <di/container/string/string_view_impl.h>
#include <di/container/string/utf8_encoding.h>
#include <di/meta/algorithm.h>

namespace di::container {
namespace detail {
    template<container::FixedString literal, size_t... indices>
    constexpr inline c8 const as_c8_buffer[sizeof...(indices)] = { static_cast<char>(literal.data()[indices])... };

    template<FixedString literal>
    struct FixedStringToUtf8StringViewFunction {
        consteval auto operator()() const {
            return function::unpack<meta::MakeIndexSequence<literal.size()>>(
                []<usize... indices>(meta::ListV<indices...>) {
                    if constexpr (literal.size() == 0) {
                        return string::StringViewImpl<string::Utf8Encoding> {};
                    } else {
                        auto span = Span { as_c8_buffer<literal, indices...> };
                        DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
                        return string::StringViewImpl<string::Utf8Encoding> { container::string::encoding::assume_valid,
                                                                              span };
                    }
                });
        }
    };
}

template<FixedString literal>
constexpr inline auto fixed_string_to_utf8_string_view = detail::FixedStringToUtf8StringViewFunction<literal> {};
}
