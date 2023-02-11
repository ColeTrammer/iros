#pragma once

#include <di/assert/prelude.h>
#include <di/container/string/string_view_impl.h>
#include <di/format/concepts/format_args.h>
#include <di/format/format_parse_context.h>
#include <di/format/formatter.h>
#include <di/function/monad/monad_try.h>
#include <di/vocab/optional/prelude.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc>
    struct VPresentEncodedContextFunction {
        using View = container::string::StringViewImpl<Enc>;

        constexpr Result<void> operator()(View format, concepts::FormatArgs auto args,
                                          concepts::FormatContext auto& context) const {
            auto parse_context = FormatParseContext<Enc> { format, args.size() };

            for (auto value : parse_context) {
                DI_ASSERT(value);

                // Literal text.
                if (value->index() == 0) {
                    for (auto code_point : util::get<0>(*value)) {
                        context.output(code_point);
                    }
                    continue;
                }

                // Format argument.
                auto arg_index = util::get<1>(*value).index;
                DI_TRY(visit(
                    [&]<typename T>(T&& value) -> Result<void> {
                        auto formatter = DI_TRY(format::formatter<meta::RemoveCVRef<T>>(parse_context));
                        return formatter(context, value);
                    },
                    args[arg_index]));
            }
            return {};
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto vpresent_encoded_context = detail::VPresentEncodedContextFunction<Enc> {};
}