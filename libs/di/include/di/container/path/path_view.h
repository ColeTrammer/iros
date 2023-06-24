#pragma once

#include <di/container/path/path_view_impl.h>
#include <di/container/string/fixed_string_to_utf8_string_view.h>
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
        consteval auto operator""_u8pv() {
            return container::Utf8PathView { container::fixed_string_to_utf8_string_view<literal>() };
        }

        consteval auto operator"" _u8pv(c8 const* data, size_t size) {
            auto span = di::Span { data, size };
            DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
            return container::Utf8PathView { container::StringView { container::string::encoding::assume_valid,
                                                                     span } };
        }

        consteval auto operator"" _pv(char const* data, size_t size) {
            auto span = di::Span { data, size };
            return container::PathView { container::TransparentStringView { span } };
        }
    }
}
}

namespace di {
using container::PathView;
using container::Utf8PathView;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_PATH_LITERALS)
using namespace di::literals::path_view_literals;
#endif
