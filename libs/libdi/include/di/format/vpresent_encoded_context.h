#pragma once

#include <di/assert/prelude.h>
#include <di/container/string/string_view_impl.h>
#include <di/format/concepts/format_args.h>
#include <di/format/formatter.h>
#include <di/vocab/optional/prelude.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc>
    struct VPresentEncodedContextFunction {
        using View = container::string::StringViewImpl<Enc>;

        constexpr void operator()(View format, concepts::FormatArgs auto args, concepts::FormatContext auto& context) const {
            auto parse_context = format::ParseContextPlaceholder {};

            size_t index = 0;
            for (auto it = format.begin(); it != format.end(); ++it) {
                auto ch = *it;
                if (ch != '{') {
                    context.output(ch);
                } else {
                    ++it;
                    DI_ASSERT(*it == '}');

                    DI_ASSERT(index < args.size());
                    auto arg = args[index++];
                    visit<void>(
                        [&]<typename T>(T&& value) {
                            auto formatter = format::formatter<meta::RemoveCVRef<T>>(parse_context);
                            formatter(context, value);
                        },
                        arg);
                }
            }
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto vpresent_encoded_context = detail::VPresentEncodedContextFunction<Enc> {};
}