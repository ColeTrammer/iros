#pragma once

#include <di/format/vpresent_encoded_context.h>
#include <di/math/divide_round_up.h>
#include <di/parser/prelude.h>

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
                       auto align = [&] {
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
                       }();

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
            return parser::integer<size_t>() % [](size_t value) {
                return Width { value };
            };
        }
    };

    struct Precision {
        size_t value;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<Precision>) {
            return (~parser::match_one('.'_m) >> parser::integer<size_t>()) % [](size_t value) {
                return Precision { value };
            };
        }
    };

    enum class StringType { String, Debug };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<StringType>) {
        using namespace integral_set_literals;
        return (-parser::match_one('s'_m || '?'_m)) % [](Optional<char32_t> ch) {
            switch (ch.value_or(U's')) {
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
        return (-parser::match_one('b'_m || 'B'_m || 'c'_m || 'd'_m || 'o'_m || 'x'_m || 'X'_m)) % [](Optional<char32_t> ch) {
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
        return (-parser::match_one('b'_m || 'B'_m || 'c'_m || 'd'_m || 'o'_m || 'x'_m || 'X'_m || '?'_m)) % [](Optional<char32_t> ch) {
            switch (ch.value_or(U'c')) {
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
        return (-parser::match_one('b'_m || 'B'_m || 'd'_m || 'o'_m || 's'_m || 'x'_m || 'X'_m)) % [](Optional<char32_t> ch) {
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
        StringType type;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<StringFormat>) {
            return (-create_parser<FillAndAlign>() >> -create_parser<Width>() >> -create_parser<Precision>() >>
                    create_parser<StringType>()) %
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
            return (-create_parser<FillAndAlign>() >> create_parser<Sign>() >> create_parser<HashTag>() >> create_parser<Zero>() >>
                    -create_parser<Width>() >> create_parser<IntegerType>()) %
                   make_from_tuple<IntegerFormat>;
        }
    };

    struct CharacterFormat {
        Optional<FillAndAlign> fill_and_align;
        Sign sign;
        HashTag hash_tag;
        Zero zero;
        Optional<Width> width;
        CharacterType type;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<CharacterFormat>) {
            return (-create_parser<FillAndAlign>() >> create_parser<Sign>() >> create_parser<HashTag>() >> create_parser<Zero>() >>
                    -create_parser<Width>() >> create_parser<CharacterType>()) %
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
            return (-create_parser<FillAndAlign>() >> create_parser<Sign>() >> create_parser<HashTag>() >> create_parser<Zero>() >>
                    -create_parser<Width>() >> create_parser<BoolType>()) %
                   make_from_tuple<BoolFormat>;
        }
    };

    struct PointerFormat {
        Optional<FillAndAlign> fill_and_align;
        Optional<Width> width;
        PointerType type;

    private:
        constexpr friend auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<PointerFormat>) {
            return (-create_parser<FillAndAlign>() >> create_parser<Width>() >> create_parser<PointerType>()) %
                   make_from_tuple<PointerFormat>;
        }
    };

    template<concepts::Encoding Enc>
    constexpr Result<void> present_string_view_to(concepts::FormatContext auto& context, Optional<FillAndAlign> fill_and_align,
                                                  Optional<size_t> width, Optional<size_t> precision, bool debug,
                                                  container::string::StringViewImpl<Enc> view) {
        using CodePoint = meta::EncodingCodePoint<Enc>;

        auto measure_code_point = [&](CodePoint) -> size_t {
            (void) debug;
            return 1;
        };

        size_t width_printed_so_far = 0;
        auto output_char = [&](CodePoint code_point) -> Result<void> {
            // Fast path when there is no precision.
            if (!precision) {
                context.output(code_point);
                return {};
            }

            // Don't print characters after the precision is exceeded.
            auto code_point_width = measure_code_point(code_point);
            if (width_printed_so_far + code_point_width > *precision) {
                return {};
            }

            context.output(code_point);
            width_printed_so_far += code_point_width;
            return {};
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
                    return { chars_to_pad, 0 };
                case FillAndAlign::Align::Center:
                    return { chars_to_pad / 2, math::divide_round_up(chars_to_pad, 2u) };
                case FillAndAlign::Align::Right:
                    return { 0, chars_to_pad };
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
    constexpr Result<void> present_character_to(concepts::FormatContext auto& context, Optional<FillAndAlign> fill_and_align,
                                                Optional<size_t> width, bool debug, c32 value) {
        auto encoding = context.encoding();
        auto as_code_units = container::string::encoding::convert_to_code_units(encoding, value);
        auto [first, last] = container::string::encoding::code_point_view(encoding, { as_code_units.data(), as_code_units.size() });
        auto as_string_view = container::string::StringViewImpl<Enc> { first, last, encoding };
        return present_string_view_to(context, fill_and_align, width, nullopt, debug, as_string_view);
    }

    template<concepts::Encoding Enc, concepts::Integral T>
    constexpr Result<void> present_integer_to(concepts::FormatContext auto& context, Optional<FillAndAlign> fill_and_align, Sign sign,
                                              HashTag hash_tag, Zero zero, Optional<size_t> width, IntegerType type, bool debug, T value) {
        if (type == IntegerType::Character) {
            return present_character_to<Enc>(context, fill_and_align, width, debug, static_cast<c32>(value));
        }

        (void) sign;
        (void) hash_tag;
        (void) zero;
        return {};
    }

    template<concepts::Encoding Enc>
    constexpr Result<void> present_formatted_to(concepts::FormatContext auto& context, Optional<FillAndAlign> fill_and_align,
                                                Optional<size_t> width, Optional<size_t> precision,
                                                container::string::StringViewImpl<Enc> format_string, auto&&... args) {
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

        return vpresent_encoded_context<Enc>(context, format_string, make_constexpr_format_args(args...));
    }
}
}