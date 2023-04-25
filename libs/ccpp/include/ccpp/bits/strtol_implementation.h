#pragma once

#include <ccpp/bits/zstring_parser.h>
#include <di/math/prelude.h>
#include <di/util/prelude.h>
#include <errno.h>

namespace ccpp {
template<di::concepts::OneOf<long, long long, unsigned long, unsigned long long> T>
T strtol(char const* __restrict string, char** __restrict end, int radix) {
    auto set_end = [&](char const* value) {
        if (end) {
            *end = const_cast<char*>(value);
        }
    };

    if (radix < 0 || radix == 1 || radix > 36) {
        set_end(string);
        errno = EINVAL;
        return 0;
    }

    auto context = di::parser::ZStringParserContext(di::ZCString(string));
    auto parser = ~di::parser::match_zero_or_more(' '_m || '\f'_m || '\n'_m || '\r'_m || '\t'_m || '\v'_m) >>
                  di::parser::integer<T, di::parser::IntegerMode::CStandard>(radix);
    auto result = parser.parse(context);

    if (!result) {
        switch (result.error()) {
            case di::parser::ZStringError::Invalid:
                errno = EINVAL;
                set_end(string);
                return 0;
            case di::parser::ZStringError::Overflow:
                errno = ERANGE;
                set_end(context.iterator_on_error().base());
                return di::NumericLimits<T>::max;
            case di::parser::ZStringError::Underflow:
                errno = ERANGE;
                set_end(context.iterator_on_error().base());
                return di::NumericLimits<T>::min;
            default:
                di::unreachable();
        }
    }

    set_end(context.begin().base().base());
    return *result;
}
}
