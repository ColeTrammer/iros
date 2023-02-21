#pragma once

#include <di/concepts/integer.h>
#include <di/math/numeric_limits.h>
#include <di/math/to_unsigned.h>
#include <di/parser/basic/match_one.h>
#include <di/parser/basic/match_one_or_more.h>
#include <di/parser/combinator/optional.h>
#include <di/parser/combinator/sequence.h>
#include <di/parser/integral_set.h>

namespace di::parser {
namespace detail {
    template<concepts::Integer T, i32 radix = 10>
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

            auto digits = [] {
                if constexpr (radix == 10) {
                    return '0'_m - '9'_m;
                } else if constexpr (radix == 8) {
                    return '0'_m - '7'_m;
                } else if constexpr (radix == 2) {
                    return '0'_m - '1'_m;
                } else if constexpr (radix == 16) {
                    return '0'_m - '9'_m || 'a'_m - 'f'_m || 'A'_m - 'F'_m;
                } else {
                    static_assert(concepts::AlwaysFalse<Nontype<radix>>, "Invalid radix for integer parser.");
                }
            }();

            auto to_digit = [](c32 code_point) {
                if constexpr (radix <= 10) {
                    return code_point - '0';
                } else if constexpr (radix <= 16) {
                    if (code_point >= 'a') {
                        return code_point - 'a' + 10;
                    } else if (code_point >= 'A') {
                        return code_point - 'A' + 10;
                    } else {
                        return code_point - '0';
                    }
                } else {
                    static_assert(concepts::AlwaysFalse<Nontype<radix>>, "Invalid radix for integer parser.");
                }
            };

            return (-match_one(sign) >> match_one_or_more(digits))
                       << [to_digit]<concepts::ParserContext Context>(
                              Context& context, auto results) -> meta::ParserContextResult<T, Context> {
                auto [sign, digits] = results;

                bool negative = false;
                if constexpr (concepts::Signed<T>) {
                    if (sign && *sign == '-') {
                        negative = true;
                    }
                }

                using Acc = meta::MakeUnsigned<T>;
                Acc result = 0;

                auto sent = container::end(digits);
                for (auto it = container::begin(digits); it != sent; ++it) {
                    auto prev_result = result;

                    result *= radix;
                    result += to_digit(*it);

                    // Unsigned overflow occurred.
                    if (prev_result > result) {
                        return Unexpected(context.make_error());
                    }
                }

                if constexpr (concepts::UnsignedInteger<T>) {
                    (void) negative;
                    return result;
                } else {
                    auto max_magnitude = math::to_unsigned(math::NumericLimits<T>::max) + negative;
                    if (result > max_magnitude) {
                        return Unexpected(context.make_error());
                    }
                    return static_cast<T>(negative ? -result : result);
                }
            };
        }
    };
}

template<concepts::Integer T, i32 radix = 10>
constexpr inline auto integer = detail::IntegerFunction<T, radix> {};

namespace detail {
    template<concepts::Integer T>
    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<T>) {
        return integer<T>();
    }
}
}
