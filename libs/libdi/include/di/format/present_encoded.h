#pragma once

#include <di/container/string/encoding.h>
#include <di/container/string/string_impl.h>
#include <di/format/concepts/formattable.h>
#include <di/format/format_context.h>
#include <di/format/format_string_impl.h>
#include <di/format/formatter.h>
#include <di/format/make_constexpr_format_args.h>
#include <di/function/template_for.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc>
    struct PresentEncodedFunction {
        template<concepts::Formattable... Args>
        constexpr concepts::MaybeFallible<container::string::StringImpl<Enc>> auto operator()(format::FormatStringImpl<Enc, Args...>,
                                                                                              Args&&... args_in) const {
            if consteval {
                auto args = make_constexpr_format_args(args_in...);
                auto context = FormatContext<Enc> {};
                auto parse_context = format::ParseContextPlaceholder {};
                function::template_for<sizeof...(Args)>([&]<size_t index>(InPlaceIndex<index>) {
                    auto arg = args[index];
                    auto formatter = format::formatter<meta::At<meta::List<Args...>, index>>(parse_context);
                    formatter(context, di::get<index>(arg));
                });
                return util::move(context).output();
            } else {
                return container::string::StringImpl<Enc> {};
            }
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto present_encoded = detail::PresentEncodedFunction<Enc> {};
}
