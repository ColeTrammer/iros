#pragma once

#include <liim/format/format_context.h>
#include <liim/format/format_parse_context.h>
#include <liim/option.h>
#include <liim/utilities.h>

namespace LIIM::Format {
namespace Detail {
    enum class Align {
        Left,
        Center,
        Right,
    };

    enum class Sign {
        MinusOrPlus,
        MinusOrSpace,
        OnlyMinus,
    };

    struct LengthSpecifier {
        size_t value;
    };

    enum class PresentationType {
        String,
        Character,
        Binary,
        BinaryUpper,
        Decimal,
        Octal,
        OctalUpper,
        Hex,
        HexUpper,
        Pointer,
        FloatHex,
        FloatHexUpper,
        FloatExponent,
        FloatExponentUpper,
        FloatFixed,
        FloatFixedUpper,
        FloatGeneral,
        FloatGeneralUpper,
    };

    class BaseOptions {
    public:
        constexpr Option<char> fill() const { return m_fill; }
        constexpr Option<Align> align() const { return m_align; }
        constexpr Sign sign() const { return m_sign; }
        constexpr bool alternate_form() const { return m_alternate_form; }
        constexpr bool zero_pad() const { return m_zero_pad; }
        constexpr Option<LengthSpecifier> width() const { return m_width; }
        constexpr Option<LengthSpecifier> precision() const { return m_precision; }
        constexpr Option<PresentationType> presentation_type() const { return m_presentation_type; }

        constexpr void set_fill(char c) { m_fill = c; }
        constexpr void set_align(Align align) { m_align = align; }
        constexpr void set_sign(Sign sign) { m_sign = sign; }
        constexpr void set_alternate_form() { m_alternate_form = true; }
        constexpr void set_zero_pad() { m_zero_pad = true; }
        constexpr void set_width(LengthSpecifier spec) { m_width = spec; }
        constexpr void set_precision(LengthSpecifier spec) { m_precision = spec; }
        constexpr void set_presentation_type(PresentationType type) { m_presentation_type = type; }

    private:
        Option<char> m_fill;
        Option<Align> m_align;
        Sign m_sign { Sign::OnlyMinus };
        bool m_alternate_form { false };
        bool m_zero_pad { false };
        Option<LengthSpecifier> m_width;
        Option<LengthSpecifier> m_precision;
        Option<PresentationType> m_presentation_type;
    };
}

struct BaseFormatter {
    Detail::BaseOptions options;

    constexpr void parse(FormatParseContext& context) {
        parse_fill_and_align(context);
        parse_flags(context);
        parse_width(context);
        parse_precision(context);
        parse_presentation_type(context);

        // Error: Expected end of format delimiter '}'
        assert(context.empty());
    }

    constexpr void parse_fill_and_align(FormatParseContext& context) {
        auto convert_char_to_align = [](char c) -> Option<Detail::Align> {
            switch (c) {
                case '<':
                    return Detail::Align::Left;
                case '^':
                    return Detail::Align::Center;
                case '>':
                    return Detail::Align::Right;
                default:
                    return {};
            }
        };

        if (context.peek(1).and_then(convert_char_to_align)) {
            options.set_fill(context.take().first());
        }

        if (auto align = context.peek().and_then(convert_char_to_align)) {
            options.set_align(*align);
            context.take();
        }
    }

    constexpr void parse_flags(FormatParseContext& context) {
        auto convert_char_to_sign = [](char c) -> Option<Detail::Sign> {
            switch (c) {
                case '+':
                    return Detail::Sign::MinusOrPlus;
                case '-':
                    return Detail::Sign::OnlyMinus;
                case ' ':
                    return Detail::Sign::MinusOrSpace;
                default:
                    return {};
            }
        };

        if (auto sign = context.peek().and_then(convert_char_to_sign)) {
            options.set_sign(*sign);
            context.take();
        }

        if (context.peek() == Option<char> { '#' }) {
            options.set_alternate_form();
            context.take();
        }

        if (context.peek() == Option<char> { '0' }) {
            options.set_zero_pad();
            context.take();
        }
    }

    constexpr void parse_width(FormatParseContext& context) {
        auto number = context.parse_number();
        if (number) {
            options.set_width({ *number });
        }
    }

    constexpr void parse_precision(FormatParseContext& context) {
        if (context.peek() != Option<char> { '.' }) {
            return;
        }
        context.take();

        auto number = context.parse_number();
        if (!number) {
            // Error: invalid precision specified
            assert(false);
        }
        options.set_precision({ *number });
    }

    constexpr void parse_presentation_type(FormatParseContext& context) {
        auto convert_char_to_presentation_type = [](char c) -> Option<Detail::PresentationType> {
            switch (c) {
                case 's':
                    return Detail::PresentationType::String;
                case 'b':
                    return Detail::PresentationType::Binary;
                case 'B':
                    return Detail::PresentationType::BinaryUpper;
                case 'c':
                    return Detail::PresentationType::Character;
                case 'd':
                    return Detail::PresentationType::Decimal;
                case 'o':
                    return Detail::PresentationType::Octal;
                case 'O':
                    return Detail::PresentationType::OctalUpper;
                case 'x':
                    return Detail::PresentationType::Hex;
                case 'X':
                    return Detail::PresentationType::HexUpper;
                case 'a':
                    return Detail::PresentationType::FloatHex;
                case 'A':
                    return Detail::PresentationType::FloatHexUpper;
                case 'e':
                    return Detail::PresentationType::FloatExponent;
                case 'E':
                    return Detail::PresentationType::FloatExponentUpper;
                case 'f':
                    return Detail::PresentationType::FloatFixed;
                case 'F':
                    return Detail::PresentationType::FloatFixedUpper;
                case 'g':
                    return Detail::PresentationType::FloatGeneral;
                case 'G':
                    return Detail::PresentationType::FloatGeneralUpper;
                case 'p':
                    return Detail::PresentationType::Pointer;
                default:
                    // Error: invalid format presentation type
                    return {};
            }
        };

        if (context.peek() == Option<char> { 'L' }) {
            // Error: locale aware formatting is unsupported
            assert(false);
        }

        if (auto type = context.peek().and_then(convert_char_to_presentation_type)) {
            options.set_presentation_type(*type);
            context.take();
        }
    }

    size_t compute_width_to_use(size_t text_width) {
        auto width_to_use = text_width;
        if (options.width()) {
            width_to_use = max(width_to_use, options.width()->value);
        }
        return width_to_use;
    }

    void do_left_pad(FormatContext& context, size_t text_width, size_t width_to_use, Detail::Align align) {
        auto left_chars = [&] {
            switch (align) {
                case Detail::Align::Center:
                    return (width_to_use - text_width) / 2;
                case Detail::Align::Right:
                    return width_to_use - text_width;
                default:
                    return 0lu;
            }
        }();

        auto padding = String::repeat(options.fill().value_or(' '), left_chars);
        context.put(padding.view());
    }

    void do_right_pad(FormatContext& context, size_t text_width, size_t width_to_use, Detail::Align align) {
        auto right_chars = [&] {
            switch (align) {
                case Detail::Align::Left:
                    return width_to_use - text_width;
                case Detail::Align::Center:
                    return (width_to_use - text_width + 1) / 2;
                default:
                    return 0lu;
            }
        }();

        auto padding = String::repeat(options.fill().value_or(' '), right_chars);
        context.put(padding.view());
    }

    void format_string_view(StringView text, FormatContext& context, Detail::Align default_align = Detail::Align::Left) {
        auto text_width = text.size();
        if (options.precision()) {
            text_width = min(text_width, options.precision()->value);
        }

        auto width_to_use = compute_width_to_use(text_width);

        auto align = options.align().value_or(default_align);
        do_left_pad(context, text_width, width_to_use, align);
        context.put(text.first(text_width));
        do_right_pad(context, text_width, width_to_use, align);
    }

    constexpr char digit_to_char(int value, bool upper) {
        auto lookup = upper ? "0123456789ABCDEF" : "0123456789abcdef";
        return lookup[value];
    }

    constexpr int radix_for_presentation_type(Detail::PresentationType type) {
        switch (type) {
            case Detail::PresentationType::Binary:
            case Detail::PresentationType::BinaryUpper:
                return 2;
            case Detail::PresentationType::Octal:
            case Detail::PresentationType::OctalUpper:
                return 8;
            case Detail::PresentationType::Decimal:
                return 10;
            case Detail::PresentationType::Hex:
            case Detail::PresentationType::HexUpper:
                return 16;
            default:
                return 10;
        }
    }

    constexpr bool is_presentation_type_upper(Detail::PresentationType type) {
        switch (type) {
            case Detail::PresentationType::BinaryUpper:
            case Detail::PresentationType::OctalUpper:
            case Detail::PresentationType::HexUpper:
            case Detail::PresentationType::FloatExponentUpper:
            case Detail::PresentationType::FloatFixedUpper:
            case Detail::PresentationType::FloatGeneralUpper:
            case Detail::PresentationType::FloatHexUpper:
                return true;
            default:
                return false;
        }
    }

    void format_unsigned_integer(uint64_t n, FormatContext& context, bool negative = false) {
        auto number_string = String {};
        if (negative) {
            number_string += String { '-' };
        } else if (options.sign() == Detail::Sign::MinusOrPlus) {
            number_string += String { '+' };
        } else if (options.sign() == Detail::Sign::MinusOrSpace) {
            number_string += String { ' ' };
        }

        auto presentation_type = options.presentation_type().value_or(Detail::PresentationType::Decimal);
        auto radix = radix_for_presentation_type(presentation_type);
        auto is_upper = is_presentation_type_upper(presentation_type);

        if (options.alternate_form()) {
            switch (presentation_type) {
                case Detail::PresentationType::Binary:
                    number_string += "0b";
                    break;
                case Detail::PresentationType::BinaryUpper:
                    number_string += "0B";
                    break;
                case Detail::PresentationType::Octal:
                    number_string += "0o";
                    break;
                case Detail::PresentationType::OctalUpper:
                    number_string += "0O";
                    break;
                case Detail::PresentationType::Hex:
                    number_string += "0x";
                    break;
                case Detail::PresentationType::HexUpper:
                    number_string += "0X";
                    break;
                default:
                    break;
            }
        }

        auto digits = String { n == 0 ? "0" : "" };
        for (; n; n /= radix) {
            digits += String { digit_to_char(n % radix, is_upper) };
        }
        digits.reverse();

        if (options.zero_pad() && !options.align() && options.width() && digits.size() + number_string.size() < options.width()->value) {
            number_string += String::repeat('0', options.width()->value - digits.size() - number_string.size());
        }

        number_string += digits;

        return format_string_view(number_string.view(), context, Detail::Align::Right);
    }

    void format_signed_integer(int64_t n, FormatContext& context) { return format_unsigned_integer(abs(n), context, n < 0); }
};
}
