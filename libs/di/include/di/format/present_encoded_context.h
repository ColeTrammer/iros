#pragma once

#include "di/container/string/encoding.h"
#include "di/container/string/string_impl.h"
#include "di/format/concepts/format_context.h"
#include "di/format/concepts/formattable.h"
#include "di/format/format_string_impl.h"
#include "di/format/make_format_args.h"
#include "di/format/vpresent_encoded_context.h"

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc>
    struct PresentEncodedContextFunction {
        template<concepts::Formattable... Args>
        constexpr auto operator()(format::FormatStringImpl<Enc, Args...> format, concepts::FormatContext auto& context,
                                  Args&&... args) const {
            return vpresent_encoded_context<Enc>(format, format::make_format_args<decltype(context)>(args...), context);
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto present_encoded_context = detail::PresentEncodedContextFunction<Enc> {};
}

namespace di {
using format::present_encoded_context;
}
