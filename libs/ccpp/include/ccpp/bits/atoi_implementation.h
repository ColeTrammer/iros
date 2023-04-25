#pragma once

#include <ccpp/bits/zstring_parser.h>

namespace ccpp {
template<di::concepts::OneOf<int, long, long long> T>
T atoi(char const* string) {
    return di::run_parser_partial(
               ~di::parser::match_zero_or_more(' '_m || '\f'_m || '\n'_m || '\r'_m || '\t'_m || '\v'_m) >>
                   di::parser::integer<T, di::parser::IntegerMode::CStandard>(10),
               di::ZCString(string))
        .value_or(0);
}
}
