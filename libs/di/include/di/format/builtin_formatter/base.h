#pragma once

#include "di/container/vector/static_vector.h"
#include "di/container/view/concat.h"
#include "di/container/view/join.h"
#include "di/container/view/transform.h"
#include "di/format/make_format_args.h"
#include "di/format/vpresent_encoded_context.h"
#include "di/function/value.h"
#include "di/math/abs.h"
#include "di/math/abs_unsigned.h"
#include "di/math/divide_round_up.h"
#include "di/parser/prelude.h"

namespace di::format {
namespace detail {
    struct FillAndAlign {
        enum class Align { Left, Center, Right };

        char32_t fill;
        Align align;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<FillAndAlign>) {
            using namespace integral_set_literals;
            return (parser::match_one(~('{'_m || '}'_m)) >> parser::match_one('<'_m || '^'_m || '>'_m)) %
                   [](concepts::TupleLike auto result) {
                       auto [fill, align_char] = result;
                       auto align = [](c32 align_char) {
                           switch (align_char) {
                               case U'<':
                                   return Align::Left;
                               case U'^':
                                   return Align::Center;
                               case U'>':
                                   return Align::Right;
                               default:
                                   util::unreachable();
                           }
                       }(align_char);

                       return FillAndAlign { fill, align };
                   };
        }
    };

    enum class Sign { Plus, Minus, Space };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<Sign>) {
        using namespace integral_set_literals;
        return (-parser::match_one('+'_m || '-'_m || ' '_m)) % [](Optional<char32_t> ch) {
            switch (ch.value_or(U'-')) {
                case U'+':
                    return Sign::Plus;
                case U'-':
                    return Sign::Minus;
                case U' ':
                    return Sign::Space;
                default:
                    util::unreachable();
            }
        };
    }

    enum class HashTag { Yes, No };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<HashTag>) {
        using namespace integral_set_literals;
        return (-parser::match_one('#'_m)) % [](Optional<char32_t> value) {
            return value.has_value() ? HashTag::Yes : HashTag::No;
        };
    }

    enum class Zero { Yes, No };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<Zero>) {
        using namespace integral_set_literals;
        return (-parser::match_one('0'_m)) % [](Optional<char32_t> value) {
            return value.has_value() ? Zero::Yes : Zero::No;
        };
    }

    struct Width {
        size_t value;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<Width>) {
            return parser::integer<size_t>(10) % [](size_t value) {
                return Width { value };
            };
        }
    };

    struct Precision {
        size_t value;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<Precision>) {
            return (~parser::match_one('.'_m) >> parser::integer<size_t>(10)) % [](size_t value) {
                return Precision { value };
            };
        }
    };

    enum class StringType { String, Debug };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<StringType>) {
        using namespace integral_set_literals;
        return (parser::match_one('s'_m || '?'_m)) % [](char32_t ch) {
            switch (ch) {
                case U's':
                    return StringType::String;
                case U'?':
                    return StringType::Debug;
                default:
                    util::unreachable();
            }
        };
    }

    enum class IntegerType { BinaryLower, BinaryUpper, Character, Decimal, Octal, HexLower, HexUpper };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<IntegerType>) {
        using namespace integral_set_literals;
        return (-parser::match_one('b'_m || 'B'_m || 'c'_m || 'd'_m || 'o'_m || 'x'_m || 'X'_m)) %
               [](Optional<char32_t> ch) {
                   switch (ch.value_or(U'd')) {
                       case U'b':
                           return IntegerType::BinaryLower;
                       case U'B':
                           return IntegerType::BinaryUpper;
                       case U'c':
                           return IntegerType::Character;
                       case U'd':
                           return IntegerType::Decimal;
                       case U'o':
                           return IntegerType::Octal;
                       case U'x':
                           return IntegerType::HexLower;
                       case U'X':
                           return IntegerType::HexUpper;
                       default:
                           util::unreachable();
                   }
               };
    }

    enum class CharacterType { BinaryLower, BinaryUpper, Character, Decimal, Octal, HexLower, HexUpper, Debug };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<CharacterType>) {
        using namespace integral_set_literals;
        return (parser::match_one('b'_m || 'B'_m || 'c'_m || 'd'_m || 'o'_m || 'x'_m || 'X'_m || '?'_m)) %
               [](char32_t ch) {
                   switch (ch) {
                       case U'b':
                           return CharacterType::BinaryLower;
                       case U'B':
                           return CharacterType::BinaryUpper;
                       case U'c':
                           return CharacterType::Character;
                       case U'd':
                           return CharacterType::Decimal;
                       case U'o':
                           return CharacterType::Octal;
                       case U'x':
                           return CharacterType::HexLower;
                       case U'X':
                           return CharacterType::HexUpper;
                       case U'?':
                           return CharacterType::Debug;
                       default:
                           util::unreachable();
                   }
               };
    }

    enum class BoolType { BinaryLower, BinaryUpper, String, Decimal, Octal, HexLower, HexUpper };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<BoolType>) {
        using namespace integral_set_literals;
        return (-parser::match_one('b'_m || 'B'_m || 'd'_m || 'o'_m || 's'_m || 'x'_m || 'X'_m)) %
               [](Optional<char32_t> ch) {
                   switch (ch.value_or(U's')) {
                       case U'b':
                           return BoolType::BinaryLower;
                       case U'B':
                           return BoolType::BinaryUpper;
                       case U'd':
                           return BoolType::Decimal;
                       case U'o':
                           return BoolType::Octal;
                       case U's':
                           return BoolType::String;
                       case U'x':
                           return BoolType::HexLower;
                       case U'X':
                           return BoolType::HexUpper;
                       default:
                           util::unreachable();
                   }
               };
    }

    enum class PointerType { HexLower };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<PointerType>) {
        using namespace integral_set_literals;
        return (-parser::match_one('p'_m)) % [](Optional<char32_t> ch) {
            switch (ch.value_or(U'p')) {
                case U'p':
                    return PointerType::HexLower;
                default:
                    util::unreachable();
            }
        };
    }

    struct StringFormat {
        Optional<FillAndAlign> fill_and_align;
        Optional<Width> width;
        Optional<Precision> precision;
        Optional<StringType> type;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<StringFormat>) {
            return (-create_parser<FillAndAlign>() >> -create_parser<Width>() >> -create_parser<Precision>() >>
                    -create_parser<StringType>()) %
                   make_from_tuple<StringFormat>;
        }
    };

    struct IntegerFormat {
        Optional<FillAndAlign> fill_and_align;
        Sign sign;
        HashTag hash_tag;
        Zero zero;
        Optional<Width> width;
        IntegerType type;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<IntegerFormat>) {
            return (-create_parser<FillAndAlign>() >> create_parser<Sign>() >> create_parser<HashTag>() >>
                    create_parser<Zero>() >> -create_parser<Width>() >> create_parser<IntegerType>()) %
                   make_from_tuple<IntegerFormat>;
        }
    };

    struct CharacterFormat {
        Optional<FillAndAlign> fill_and_align;
        Sign sign;
        HashTag hash_tag;
        Zero zero;
        Optional<Width> width;
        Optional<CharacterType> type;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<CharacterFormat>) {
            return (-create_parser<FillAndAlign>() >> create_parser<Sign>() >> create_parser<HashTag>() >>
                    create_parser<Zero>() >> -create_parser<Width>() >> -create_parser<CharacterType>()) %
                   make_from_tuple<CharacterFormat>;
        }
    };

    struct BoolFormat {
        Optional<FillAndAlign> fill_and_align;
        Sign sign;
        HashTag hash_tag;
        Zero zero;
        Optional<Width> width;
        BoolType type;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<BoolFormat>) {
            return (-create_parser<FillAndAlign>() >> create_parser<Sign>() >> create_parser<HashTag>() >>
                    create_parser<Zero>() >> -create_parser<Width>() >> create_parser<BoolType>()) %
                   make_from_tuple<BoolFormat>;
        }
    };

    struct PointerFormat {
        Optional<FillAndAlign> fill_and_align;
        Optional<Width> width;
        PointerType type;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<PointerFormat>) {
            return (-create_parser<FillAndAlign>() >> -create_parser<Width>() >> create_parser<PointerType>()) %
                   make_from_tuple<PointerFormat>;
        }
    };

    template<concepts::Encoding Enc>
    constexpr auto present_string_view_to(concepts::FormatContext auto& context, Optional<FillAndAlign> fill_and_align,
                                          Optional<size_t> width, Optional<size_t> precision, bool debug,
                                          container::string::StringViewImpl<Enc> view_in,
                                          char32_t delimit_code_point = U'"') -> Result<void> {
        using CodePoint = meta::EncodingCodePoint<Enc>;

        auto delimit = lift_bool(debug) % function::value(delimit_code_point);

        auto map_to_debug_char = [&](CodePoint p) -> di::StaticVector<c32, di::Constexpr<4>> {
            auto result = di::StaticVector<c32, di::Constexpr<4>> {};
            if (!debug) {
                (void) result.push_back(c32(p));
            } else {
                // Special escaped chars. These are represented as \P.
                if (c32(p) == '\0') {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(U'0');
                } else if (c32(p) == '\a') {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(U'a');
                } else if (c32(p) == '\b') {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(U'b');
                } else if (c32(p) == '\f') {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(U'f');
                } else if (c32(p) == '\n') {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(U'n');
                } else if (c32(p) == '\r') {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(U'r');
                } else if (c32(p) == '\t') {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(U't');
                } else if (c32(p) == '\v') {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(U'v');
                } else if (c32(p) == '\\') {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(U'\\');
                }
                // The delimiter is also escaped. For instance: " will become \".
                else if (c32(p) == delimit_code_point) {
                    (void) result.push_back(U'\\');
                    (void) result.push_back(delimit_code_point);
                }
                // Control characters should not be printed directly in debug mode.
                // This range accounts for C0 and C1 Unicode control characters.
                else if (p <= 31 || p == 127 || (p >= 0x80 && p <= 0x9F)) {
                    auto v1 = (u8(p) / 16) & 0xF;
                    auto v2 = (u8(p) % 16);

                    (void) result.push_back(U'\\');
                    (void) result.push_back(U'x');
                    (void) result.push_back((v1 >= 10) ? ('a' + (v1 - 10)) : ('0' + v1));
                    (void) result.push_back((v2 >= 10) ? ('a' + (v2 - 10)) : ('0' + v2));
                } else {
                    (void) result.push_back(c32(p));
                }
            }
            return result;
        };

        auto view = view::concat(delimit, view::join(view::transform(view_in, map_to_debug_char)), delimit);

        auto measure_code_point = [&](CodePoint) -> size_t {
            // TODO: use east asian width field.
            // TODO: measure grapheme clusters instead of measuring width code point by code point.
            return 1;
        };

        auto do_output_char = [&](CodePoint code_point) -> Result<void> {
            (void) context.output(code_point);
            return {};
        };

        size_t width_printed_so_far = 0;

        auto output_char = [&](CodePoint code_point) -> Result<void> {
            // Fast path when there is no precision.
            if (!precision) {
                return do_output_char(code_point);
            }

            // Don't print characters after the precision is exceeded.
            auto code_point_width = measure_code_point(code_point);
            if (width_printed_so_far + code_point_width > *precision) {
                return {};
            }

            auto result = do_output_char(code_point);
            width_printed_so_far += code_point_width;
            return result;
        };

        if (!width) {
            return container::sequence(view, output_char);
        }

        auto total_width = view | view::transform(measure_code_point) | container::sum;
        if (total_width >= *width) {
            return container::sequence(view, output_char);
        }

        auto align = fill_and_align.transform(&FillAndAlign::align).value_or(FillAndAlign::Align::Left);
        auto fill_code_point = fill_and_align.transform(&FillAndAlign::fill).value_or(U' ');

        auto chars_to_pad = (*width - total_width) / measure_code_point(fill_code_point);
        auto [left_pad, right_pad] = [&]() -> Tuple<size_t, size_t> {
            switch (align) {
                case FillAndAlign::Align::Left:
                    return { 0, chars_to_pad };
                case FillAndAlign::Align::Center:
                    return { chars_to_pad / 2, math::divide_round_up(chars_to_pad, 2U) };
                case FillAndAlign::Align::Right:
                    return { chars_to_pad, 0 };
                default:
                    util::unreachable();
            }
        }();

        auto do_pad = [&](auto) {
            return output_char(fill_code_point);
        };

        DI_TRY(container::sequence(view::range(left_pad), do_pad));
        DI_TRY(container::sequence(view, output_char));
        return container::sequence(view::range(right_pad), do_pad);
    }

    template<concepts::Encoding Enc>
    constexpr auto present_character_to(concepts::FormatContext auto& context, Optional<FillAndAlign> fill_and_align,
                                        Optional<size_t> width, bool debug, c32 value) -> Result<void> {
        auto encoding = context.encoding();
        auto as_code_units = container::string::encoding::convert_to_code_units(encoding, value);
        auto [first, last] =
            container::string::encoding::code_point_view(encoding, { as_code_units.data(), as_code_units.size() });
        auto as_string_view = container::string::StringViewImpl<Enc> { first, last, encoding };
        return present_string_view_to(context, fill_and_align, width, nullopt, debug, as_string_view, U'\'');
    }

    template<concepts::Encoding Enc, concepts::Integral T>
    constexpr auto present_integer_to(concepts::FormatContext auto& context, Optional<FillAndAlign> fill_and_align,
                                      Sign sign, HashTag hash_tag, Zero zero, Optional<size_t> width, IntegerType type,
                                      bool debug, T value) -> Result<void> {
        if (type == IntegerType::Character) {
            return present_character_to<Enc>(context, fill_and_align, width, debug, static_cast<c32>(value));
        }

        using CodePoint = meta::EncodingCodePoint<Enc>;

        // The maximum number of digits a number can have is 64 (u64::max() printed in binary).
        // Add 3 extra characters to account for a prefix, like -0x.
        auto buffer = container::string::StringImpl<
            Enc, container::StaticVector<meta::EncodingCodeUnit<Enc>, meta::Constexpr<67ZU>>> {};

        using UnsignedType = meta::MakeUnsigned<T>;
        auto as_unsigned = math::abs_unsigned(value);

        auto const negative = [&] {
            if constexpr (concepts::Signed<T>) {
                return value < 0;
            }
            return false;
        }();

        auto do_sign = [&] {
            switch (sign) {
                case Sign::Minus:
                    if (negative) {
                        (void) buffer.push_back(CodePoint('-'));
                    }
                    break;
                case Sign::Plus:
                    if (negative) {
                        (void) buffer.push_back(CodePoint('-'));
                    } else {
                        (void) buffer.push_back(CodePoint('+'));
                    }
                    break;
                case Sign::Space:
                    if (negative) {
                        (void) buffer.push_back(CodePoint('-'));
                    } else {
                        (void) buffer.push_back(CodePoint(' '));
                    }
                    break;
            }
        };

        auto do_prefix = [&] {
            if (hash_tag == HashTag::No) {
                return;
            }
            switch (type) {
                case IntegerType::BinaryLower:
                    (void) buffer.push_back(CodePoint('0'));
                    (void) buffer.push_back(CodePoint('b'));
                    break;
                case IntegerType::BinaryUpper:
                    (void) buffer.push_back(CodePoint('0'));
                    (void) buffer.push_back(CodePoint('B'));
                    break;
                case IntegerType::Octal:
                    (void) buffer.push_back(CodePoint('0'));
                    break;
                case IntegerType::HexLower:
                    (void) buffer.push_back(CodePoint('0'));
                    (void) buffer.push_back(CodePoint('x'));
                    break;
                case IntegerType::HexUpper:
                    (void) buffer.push_back(CodePoint('0'));
                    (void) buffer.push_back(CodePoint('X'));
                    break;
                default:
                    break;
            }
        };

        bool zero_pad = zero == Zero::Yes && !fill_and_align;

        do_sign();
        do_prefix();

        if (zero_pad) {
            for (auto ch : buffer) {
                context.output(ch);
            }
            auto code_points = math::to_unsigned(container::distance(buffer));
            if (width && *width > code_points) {
                *width -= code_points;
            }
            buffer.clear();
        }

        auto const radix = [&] -> UnsignedType {
            switch (type) {
                case IntegerType::BinaryLower:
                case IntegerType::BinaryUpper:
                    return 2;
                case IntegerType::Octal:
                    return 8;
                case IntegerType::HexLower:
                case IntegerType::HexUpper:
                    return 16;
                default:
                    return 10;
            }
        }();

        auto to_digit = [&](UnsignedType value) -> meta::EncodingCodePoint<Enc> {
            switch (type) {
                case IntegerType::HexLower:
                    return (value >= 10) ? ('a' + (value - 10)) : ('0' + value);
                case IntegerType::HexUpper:
                    return (value >= 10) ? ('A' + (value - 10)) : ('0' + value);
                default:
                    return ('0' + value);
            }
        };

        UnsignedType strength = 1;
        for (auto x = as_unsigned; x / strength >= radix; strength *= radix) {}

        for (; strength; strength /= radix) {
            (void) buffer.push_back(to_digit((as_unsigned / strength) % radix));
        }

        auto backup_fill_and_align = FillAndAlign { zero_pad ? U'0' : U' ', FillAndAlign::Align::Right };
        return present_string_view_to<Enc>(context, fill_and_align.value_or(backup_fill_and_align), width, nullopt,
                                           false, buffer);
    }

    template<concepts::Encoding Enc>
    constexpr auto present_formatted_to(concepts::FormatContext auto& context, Optional<FillAndAlign> fill_and_align,
                                        Optional<size_t> width, Optional<size_t> precision,
                                        container::string::StringViewImpl<Enc> format_string, auto&&... args)
        -> Result<void> {
        // If there is no width, fill_and_align is ignored, so no temporary buffer is needed.

        // Precision in this case refers to the upper bound on the text width to be printed.
        // Handle precision using a modified context which caps to number of characters printed,
        // in the case where there is no width.

        // If width is present, we must first collect the input into a temporary buffer before presenting.
        // Performing 2 passes is not viable, as we may be printing InputContainers, which are only valid
        // for a single pass, and anyway, iterating over a large vector twice should be avoided.

        // The temporary buffer should include a sizable inline capacity (~256 bytes), to prevent heap
        // allocation except for in extreme cases. Note that if the width is smaller than 256, formatting
        // can be successful without ever allocating. However, the temporary buffer should be customizable,
        // so that in kernel context, allocation can never happen (and perhaps less stack size should be
        // used, since the kernel may only have 4096 bytes of stack space).

        DI_ASSERT(!width);
        DI_ASSERT(!precision);
        (void) fill_and_align;

        return vpresent_encoded_context<Enc>(context, format_string,
                                             format::make_format_args<decltype(context)>(args...));
    }
}
}
