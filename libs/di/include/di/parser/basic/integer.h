#pragma once

#include <di/math/intcmp/checked.h>
#include <di/math/numeric_limits.h>
#include <di/math/to_unsigned.h>
#include <di/meta/language.h>
#include <di/parser/basic/match_one.h>
#include <di/parser/basic/match_one_or_more.h>
#include <di/parser/combinator/optional.h>
#include <di/parser/combinator/sequence.h>
#include <di/parser/integral_set.h>
#include <di/parser/make_error.h>
#include <di/parser/meta/parser_context_result.h>
#include <di/parser/parser_base.h>
#include <di/types/char.h>
#include <di/util/get.h>
#include <di/vocab/expected/unexpected.h>
#include <di/vocab/tuple/make_tuple.h>

namespace di::parser {
enum class IntegerError {
    Overflow,
    Underflow,
};

/// @brief Mode to use when parsing an integer
///
/// For compatibility with C, applications can pass the CStandard mode. This works as defined by strtol() and strtoul().
/// However, this is often not clear. In the Improved mode, the parser accepts 0b and 0o as additional prefixes, and
/// doesn't consider a leading 0 to be an octal prefix. Additionally, you cannot negate an unsigned integer in the
/// Improved mode.
enum class IntegerMode {
    Improved,
    CStandard,
};

namespace detail {
    template<IntegerMode mode>
    class MatchIntegerPrefixParser : public ParserBase<MatchIntegerPrefixParser<mode>> {
    public:
        constexpr explicit MatchIntegerPrefixParser(int radix) : m_radix(radix) {}

        template<concepts::ParserContext Context>
        constexpr auto parse(Context& context) const -> meta::ParserContextResult<
            vocab::Tuple<
                int, meta::Reconstructed<Context, meta::ContainerIterator<Context>, meta::ContainerIterator<Context>>>,
            Context> {
            auto parse_digits = [&](int inferred_radix)
                -> meta::ParserContextResult<
                    vocab::Tuple<int, meta::Reconstructed<Context, meta::ContainerIterator<Context>,
                                                          meta::ContainerIterator<Context>>>,
                    Context> {
                auto valid_digit = [&](c32 code_point) {
                    if (inferred_radix <= 10) {
                        return code_point >= U'0' && code_point < U'0' + inferred_radix;
                    }
                    return (code_point >= U'0' && code_point <= U'9') ||
                           (code_point >= U'a' && code_point < U'a' + inferred_radix - 10) ||
                           (code_point >= U'A' && code_point < U'A' + inferred_radix - 10);
                };

                auto start = container::begin(context);
                auto sent = container::end(context);

                auto it = start;
                while (it != sent && valid_digit(*it)) {
                    ++it;
                }

                if (it == start) {
                    return vocab::Unexpected(context.make_error());
                }

                context.advance(it);
                return vocab::make_tuple(inferred_radix, container::reconstruct(in_place_type<Context>, start, it));
            };

            // Only octal and hexadecimal numbers can have a prefix.
            if (m_radix != 0 && m_radix != 8 && m_radix != 16 && m_radix != 2) {
                return parse_digits(m_radix);
            }

            auto start = container::begin(context);
            auto sent = container::end(context);

            auto it = start;
            if (it == sent) {
                return parse_digits(m_radix == 0 ? 10 : m_radix);
            }

            auto ch = *it;
            if (ch != U'0') {
                return parse_digits(m_radix == 0 ? 10 : m_radix);
            }

            // If the string is exactly "0", then it is not a prefix. Just parse it as a decimal number, which will
            // result in the value of 0.
            if (++it == sent) {
                return parse_digits(10);
            }

            if constexpr (mode == IntegerMode::CStandard) {
                if (m_radix == 8) {
                    // Since a 0 has been encountered, infer the radix to be 8.
                    return parse_digits(8);
                }
            }

            ch = *it;
            if (ch == U'x' || ch == U'X') {
                context.advance(++it);
                return parse_digits(16);
            }
            if (ch == U'b' || ch == U'B') {
                context.advance(++it);
                return parse_digits(2);
            }
            if constexpr (mode == IntegerMode::Improved) {
                if (ch == U'o' || ch == U'O') {
                    context.advance(++it);
                    return parse_digits(8);
                }
                return parse_digits(m_radix == 0 ? 10 : m_radix);
            }
            return parse_digits(m_radix == 0 ? 8 : m_radix);
        }

    private:
        int m_radix { 10 };
    };

    template<concepts::Integer T, IntegerMode mode = IntegerMode::Improved>
    struct IntegerFunction {
        constexpr auto operator()(int radix = 0) const {
            using namespace di::literals;

            DI_ASSERT(radix == 0 || (radix >= 2 && radix <= 36));

            auto sign = [] {
                if constexpr (concepts::Signed<T> || mode == IntegerMode::CStandard) {
                    return '-'_m || '+'_m;
                } else {
                    return '+'_m;
                }
            }();

            return (-match_one(sign) >> MatchIntegerPrefixParser<mode>(radix))
                       << []<concepts::ParserContext Context>(Context& context,
                                                              auto results) -> meta::ParserContextResult<T, Context> {
                auto [sign, number_part] = results;

                // NOTE: We can't use structured bindings here because clang-16 doesn't think its a constant expression.
                auto inferred_radix = util::get<0>(number_part);
                auto digits = util::get<1>(number_part);

                bool negative = false;
                if constexpr (concepts::Signed<T> || mode == IntegerMode::CStandard) {
                    if (sign && *sign == '-') {
                        negative = true;
                    }
                }

                auto to_digit = [&](c32 code_point) {
                    if (inferred_radix <= 10) {
                        return code_point - U'0';
                    }
                    if (code_point >= U'a') {
                        return code_point - U'a' + 10;
                    }
                    if (code_point >= U'A') {
                        return code_point - U'A' + 10;
                    }
                    return code_point - U'0';
                };

                auto overflow_error = [&] {
                    if constexpr (mode == IntegerMode::CStandard && !concepts::Signed<T>) {
                        return IntegerError::Overflow;
                    }
                    return negative ? IntegerError::Underflow : IntegerError::Overflow;
                }();

                using U = meta::MakeUnsigned<T>;
                auto result = math::Checked<U>(0);

                auto sent = container::end(digits);
                for (auto it = container::begin(digits); it != sent; ++it) {
                    result *= inferred_radix;
                    result += to_digit(*it);
                }

                if (result.invalid()) {
                    return Unexpected(parser::make_error(context, overflow_error, sent));
                }
                auto value = *result.value();
                if constexpr (concepts::Signed<T>) {
                    auto max_magnitude = math::to_unsigned(math::NumericLimits<T>::max) + negative;
                    if (value > max_magnitude) {
                        return Unexpected(parser::make_error(context, overflow_error, sent));
                    }
                    return static_cast<T>(negative ? -value : value);
                } else if constexpr (mode == IntegerMode::CStandard) {
                    return negative ? -value : value;
                }
                return value;
            };
        }
    };
}

template<concepts::Integer T, IntegerMode mode = IntegerMode::Improved>
constexpr inline auto integer = detail::IntegerFunction<T, mode> {};

namespace detail {
    template<concepts::Integer T>
    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<T>) {
        return integer<T>();
    }
}
}
