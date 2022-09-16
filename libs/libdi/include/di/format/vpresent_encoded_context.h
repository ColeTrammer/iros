#pragma once

#include <di/container/string/string_view_impl.h>
#include <di/format/concepts/format_args.h>
#include <di/format/formatter.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc>
    struct VPresentEncodedContextFunction {
        using View = container::string::StringViewImpl<Enc>;

        constexpr void operator()(View, concepts::FormatArgs auto args, concepts::FormatContext auto& context) const {
            auto parse_context = format::ParseContextPlaceholder {};
            for (size_t i = 0; i < args.size(); i++) {
                auto arg = args[i];
                visit<void>(
                    [&]<typename T>(T&& value) {
                        auto formatter = format::formatter<meta::RemoveCVRef<T>>(parse_context);
                        formatter(context, value);
                    },
                    arg);
            }
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto vpresent_encoded_context = detail::VPresentEncodedContextFunction<Enc> {};
}