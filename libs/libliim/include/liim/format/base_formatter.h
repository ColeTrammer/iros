#pragma once

#include <liim/format/format_context.h>
#include <liim/format/format_parse_context.h>
#include <liim/maybe.h>
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
        constexpr Maybe<char> fill() const { return m_fill; }
        constexpr Align align() const { return m_align; }
        constexpr Sign sign() const { return m_sign; }
        constexpr bool alternate_form() const { return m_alternate_form; }
        constexpr bool zero_pad() const { return m_zero_pad; }
        constexpr Maybe<LengthSpecifier> width() const { return m_width; }
        constexpr Maybe<LengthSpecifier> precision() const { return m_precision; }
        constexpr Maybe<PresentationType> presentation_type() const { return m_presentation_type; }

        constexpr void set_fill(char c) { m_fill = c; }
        constexpr void set_align(Align align) { m_align = align; }
        constexpr void set_sign(Sign sign) { m_sign = sign; }
        constexpr void set_alternate_form() { m_alternate_form = true; }
        constexpr void set_zero_pad() { m_zero_pad = true; }
        constexpr void set_width(LengthSpecifier spec) { m_width = spec; }
        constexpr void set_precision(LengthSpecifier spec) { m_precision = spec; }
        constexpr void set_presentation_type(PresentationType type) { m_presentation_type = type; }

    private:
        Maybe<char> m_fill;
        Align m_align { Align::Left };
        Sign m_sign { Sign::OnlyMinus };
        bool m_alternate_form { false };
        bool m_zero_pad { false };
        Maybe<LengthSpecifier> m_width;
        Maybe<LengthSpecifier> m_precision;
        Maybe<PresentationType> m_presentation_type;
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
        auto convert_char_to_align = [](char c) -> Maybe<Detail::Align> {
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
        auto convert_char_to_sign = [](char c) -> Maybe<Detail::Sign> {
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
        }

        if (context.peek() == Maybe<char> { '#' }) {
            options.set_alternate_form();
            context.take();
        }

        if (context.peek() == Maybe<char> { '0' }) {
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
        if (context.peek() != Maybe<char> { '.' }) {
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
        auto convert_char_to_presentation_type = [](char c) -> Maybe<Detail::PresentationType> {
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

        if (context.peek() == Maybe<char> { 'L' }) {
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

    void do_left_pad(FormatContext& context, size_t text_width, size_t width_to_use) {
        auto left_chars = [&] {
            switch (options.align()) {
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

    void do_right_pad(FormatContext& context, size_t text_width, size_t width_to_use) {
        auto right_chars = [&] {
            switch (options.align()) {
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

    void format_string_view(StringView text, FormatContext& context) {
        auto text_width = text.size();
        if (options.precision()) {
            text_width = min(text_width, options.precision()->value);
        }

        auto width_to_use = compute_width_to_use(text_width);

        do_left_pad(context, text_width, width_to_use);
        context.put(text.first(text_width));
        do_right_pad(context, text_width, width_to_use);
    }
};
}
