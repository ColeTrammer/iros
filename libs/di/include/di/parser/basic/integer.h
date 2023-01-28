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

            return (-match_one(sign) >> match_one_or_more(digits))
                       << []<concepts::ParserContext Context>(Context& context,
                                                              auto results) -> meta::ParserContextResult<T, Context> {
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

                    // FIXME: support other radixes.
                    result *= 10;
                    result += (*it - U'0');

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

template<concepts::Integer T>
constexpr inline auto integer = detail::IntegerFunction<T> {};

namespace detail {
    template<concepts::Integer T>
    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<T>) {
        return integer<T>();
    }
}
}