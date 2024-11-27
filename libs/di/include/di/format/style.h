#pragma once

#include <di/format/bounded_format_context.h>
#include <di/format/present_encoded_context.h>

namespace di::format::style {
/// @brief Represent a ANSI terminal color.
///
/// These correspond to the CSI SGR parameters, as documented here:
/// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h4-Functions-using-CSI-_-ordered-by-the-final-character-lparen-s-rparen:CSI-Pm-m.1CA7
enum class Color {
    Black = 0,
    Red = 1,
    Green = 2,
    Yellow = 3,
    Blue = 4,
    Magenta = 5,
    Cyan = 6,
    White = 7,
    Default = 9,
};

enum class Effect {
    None = 0,
    Bold = 1,
    Faint = 2,
    Italic = 3,
    Underline = 4,
    Blink = 5,
    Inverted = 7,
    StrikeThrough = 9,
    DoubleUnderline = 21,
};

class BackgroundColor {
public:
    constexpr explicit BackgroundColor(Color color) : m_color(color) {}

    constexpr auto color() const -> Color { return m_color; }

private:
    Color m_color { Color::Default };
};

class Style {
public:
    constexpr Style() = default;

    constexpr Style(Color foreground) : m_foreground(foreground) {}
    constexpr Style(BackgroundColor background) : m_background(background) {}
    constexpr Style(Effect effect) : m_effect(effect) {}

    constexpr explicit Style(Color foreground, BackgroundColor background, Effect effect)
        : m_foreground(foreground), m_background(background), m_effect(effect) {}

    constexpr auto foreground() const -> Color { return m_foreground; }
    constexpr auto background() const -> Color { return m_background.color(); }
    constexpr auto effect() const -> Effect { return m_effect; }

    template<concepts::Encoding Enc>
    constexpr auto render_to_ansi_escapes() const {
        // Use a bounded context to avoid allocating a string.
        using TargetContext = format::BoundedFormatContext<Enc, meta::Constexpr<32zu>>;

        auto context = TargetContext {};
        (void) present_encoded_context<Enc>("\033[{};{};{}m"_sv, context, util::to_underlying(effect()),
                                            util::to_underlying(foreground()) + 30,
                                            util::to_underlying(background()) + 40);

        auto context_for_reset = TargetContext {};
        (void) present_encoded_context<Enc>("\033[0m"_sv, context_for_reset);

        return Tuple(util::move(context).output(), util::move(context_for_reset).output());
    }

private:
    Color m_foreground { Color::Default };
    BackgroundColor m_background { Color::Default };
    Effect m_effect { Effect::None };
};

constexpr auto operator|(Color color, BackgroundColor background) {
    return Style(color, background, Effect::None);
}

constexpr auto operator|(BackgroundColor background, Color color) {
    return Style(color, background, Effect::None);
}

constexpr auto operator|(Color color, Effect effect) {
    return Style(color, BackgroundColor(Color::Default), effect);
}

constexpr auto operator|(Effect effect, Color color) {
    return Style(color, BackgroundColor(Color::Default), effect);
}

constexpr auto operator|(BackgroundColor background, Effect effect) {
    return Style(Color::Default, background, effect);
}

constexpr auto operator|(Effect effect, BackgroundColor background) {
    return Style(Color::Default, background, effect);
}

constexpr auto operator|(Style style, Color color) {
    return Style(color, BackgroundColor(style.background()), style.effect());
}

constexpr auto operator|(Color color, Style style) {
    return Style(color, BackgroundColor(style.background()), style.effect());
}

constexpr auto operator|(Style style, BackgroundColor background) {
    return Style(style.foreground(), background, style.effect());
}

constexpr auto operator|(BackgroundColor background, Style style) {
    return Style(style.foreground(), background, style.effect());
}

constexpr auto operator|(Style style, Effect effect) {
    return Style(style.foreground(), BackgroundColor(style.background()), effect);
}

constexpr auto operator|(Effect effect, Style style) {
    return Style(style.foreground(), BackgroundColor(style.background()), effect);
}

template<concepts::Formattable T>
class Styled {
public:
    constexpr explicit Styled(T&& argument, Style style) : m_argument(util::forward<T>(argument)), m_style(style) {}

private:
    template<concepts::Encoding Enc>
    constexpr friend auto tag_invoke(types::Tag<format::formatter_in_place>, InPlaceType<Styled>,
                                     FormatParseContext<Enc>& parse_context, bool debug) {
        return format::formatter<T, Enc>(parse_context, debug) % [](concepts::CopyConstructible auto formatter) {
            return [=]<concepts::FormatContext Context>(Context& context, Styled const& value) {
                if constexpr (requires { typename Context::SupportsStyle; }) {
                    return context.with_style(value.m_style, [&] {
                        return formatter(context, value.m_argument);
                    });
                } else {
                    return formatter(context, value.m_argument);
                }
            };
        };
    }

    T&& m_argument;
    Style m_style {};
};

template<typename T>
Styled(T&&, Style) -> Styled<T&&>;
}

namespace di::format {
using style::BackgroundColor;
using style::Color;
using style::Effect;
using style::Style;
using style::Styled;
}

namespace di {
using format::Styled;

using FormatColor = format::Color;
using FormatBackgroundColor = format::BackgroundColor;
using FormatEffect = format::Effect;
}
