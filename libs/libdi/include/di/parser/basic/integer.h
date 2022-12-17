#pragma once

#include <di/concepts/integer.h>
#include <di/parser/basic/match_one.h>
#include <di/parser/basic/match_one_or_more.h>
#include <di/parser/combinator/optional.h>
#include <di/parser/combinator/sequence.h>
#include <di/parser/integral_set.h>

namespace di::parser {
namespace detail {

    template<concepts::Integer T>
    struct IntegerFunction {
        constexpr auto operator()() const {
            using namespace di::literals;

            auto sign = [] {
                if constexpr (concepts::Signed<T>) {
                    return '-'_m || '+'_m;
                } else {
                    return '+'_m;
                }
            }();
            auto digits = '0'_m - '9'_m;

            return and_then(sequence(optional(match_one(sign)), match_one_or_more(digits)),
                            []<concepts::ParserContext Context>(Context&, auto results) -> meta::ParserContextResult<T, Context> {
                                auto [sign, digits] = results;

                                T sign_value = 1;
                                if constexpr (concepts::Signed<T>) {
                                    if (sign && *sign == '-') {
                                        sign_value = -1;
                                    }
                                }

                                T result = 0;

                                auto sent = container::end(digits);
                                for (auto it = container::begin(digits); it != sent; ++it) {
                                    // FIXME: support other radixes.
                                    // FIXME: check for overflow.
                                    result *= 10;
                                    result += (*it - U'0');
                                }

                                // FIXME: handle the INT_MIN case correctly.
                                return result * sign_value;
                            });
        }
    };
}

template<concepts::Integer T>
constexpr inline auto integer = detail::IntegerFunction<T> {};

namespace detail {
    template<concepts::Integer T>
    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<T>, concepts::ParserContext auto&) {
        return integer<T>();
    }
}
}