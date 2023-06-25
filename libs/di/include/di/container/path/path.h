#pragma once

#include <di/container/path/path_impl.h>
#include <di/container/path/path_view.h>
#include <di/container/string/string.h>
#include <di/container/string/transparent_encoding.h>
#include <di/container/string/utf8_encoding.h>

namespace di::container {
using Path = PathImpl<TransparentString>;
using Utf8Path = PathImpl<String>;
}

namespace di {
inline namespace literals {
    inline namespace path_literals {
        template<container::FixedString literal>
        constexpr auto operator""_u8p() {
            auto view = container::Utf8PathView { container::fixed_string_to_utf8_string_view<literal>() };
            return view.to_owned();
        }

        constexpr auto operator"" _u8p(c8 const* data, size_t size) {
            auto span = di::Span { data, size };
            DI_ASSERT(container::string::encoding::validate(container::string::Utf8Encoding(), span));
            auto view =
                container::Utf8PathView { container::StringView { container::string::encoding::assume_valid, span } };
            return view.to_owned();
        }

        constexpr auto operator"" _p(char const* data, size_t size) {
            auto span = di::Span { data, size };
            auto view = container::PathView { container::TransparentStringView { span } };
            return view.to_owned();
        }
    }
}
}

namespace di {
using container::Path;
using container::Utf8Path;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_PATH_LITERALS)
using namespace di::literals::path_literals;
#endif
