#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/string/string_view_impl.h>
#include <di/format/concepts/format_arg.h>
#include <di/format/concepts/format_args.h>
#include <di/format/format_arg.h>
#include <di/format/format_args.h>
#include <di/format/format_parse_context.h>
#include <di/format/formatter.h>
#include <di/function/monad/monad_try.h>
#include <di/vocab/optional/prelude.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc, typename Var>
    constexpr Result<void> do_format(Var&& variant, FormatParseContext<Enc>& parse_context,
                                     concepts::FormatContext auto& context, bool debug = false) {
        return visit(
            [&]<typename T>(T&& value) -> Result<void> {
                if constexpr (concepts::InstanceOf<meta::RemoveCVRef<T>, ErasedArg>) {
                    return value.do_format(parse_context, context, debug);
                } else {
                    auto formatter = DI_TRY(format::formatter<meta::RemoveCVRef<T>, Enc>(parse_context, debug));
                    return formatter(context, value);
                }
            },
            variant);
    }

    template<concepts::Encoding Enc>
    struct VPresentEncodedContextFunction {
        using View = container::string::StringViewImpl<Enc>;

        template<concepts::FormatArg Arg>
        constexpr Result<void> operator()(View format, FormatArgs<Arg> args,
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
                DI_TRY(do_format(args[arg_index], parse_context, context));
            }
            return {};
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto vpresent_encoded_context = detail::VPresentEncodedContextFunction<Enc> {};
}
