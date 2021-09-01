#include <liim/string_view.h>
#include <tinput/io_terminal.h>
#include <tinput/terminal_glyph.h>
#include <tinput/terminal_renderer.h>

namespace TInput {
static TerminalGlyph box_glyphs_array[6][8] = {
    {
        { "┏", 1 },
        { "━", 1 },
        { "┓", 1 },
        { "┃", 1 },
        { "┛", 1 },
        { "━", 1 },
        { "┗", 1 },
        { "┃", 1 },
    },
    {
        { "┌", 1 },
        { "─", 1 },
        { "┐", 1 },
        { "│", 1 },
        { "┘", 1 },
        { "─", 1 },
        { "└", 1 },
        { "│", 1 },
    },
    {
        { "╔", 1 },
        { "═", 1 },
        { "╗", 1 },
        { "║", 1 },
        { "╝", 1 },
        { "═", 1 },
        { "╚", 1 },
        { "║", 1 },
    },
    {
        { "┏", 1 },
        { "┅", 1 },
        { "┓", 1 },
        { "┇", 1 },
        { "┛", 1 },
        { "┅", 1 },
        { "┗", 1 },
        { "┇", 1 },
    },
    {
        { "┌", 1 },
        { "╌", 1 },
        { "┐", 1 },
        { "╎", 1 },
        { "┘", 1 },
        { "╌", 1 },
        { "└", 1 },
        { "╎", 1 },
    },
    {
        { "+", 1 },
        { "-", 1 },
        { "+", 1 },
        { "|", 1 },
        { "+", 1 },
        { "-", 1 },
        { "+", 1 },
        { "|", 1 },
    },
};

void TerminalRenderer::draw_rect(const Rect& rect_in, Maybe<Color> color, BoxStyle box_style) {
    if (rect_in.width() < 1 || rect_in.height() < 1) {
        return clear_rect(rect_in, color);
    }

#ifdef __os_2__
    // FIXME: support unicode in the system terminal
    box_style = BoxStyle::Ascii;
#endif /* __os_2__ */

    auto rect = translate(rect_in);
    auto style = TerminalTextStyle { .foreground = color, .background = {}, .bold = false };

    auto glyphs = box_glyphs_array[static_cast<uint8_t>(box_style)];

    // Top Left
    if (auto point = rect.top_left(); in_bounds(point)) {
        m_io_terminal.put_glyph(point, glyphs[0], style);
    }

    // Top Edge
    for (auto x = rect.left() + 1; x < rect.right() - 1; x++) {
        auto point = Point { x, rect.top() };
        if (in_bounds(point)) {
            m_io_terminal.put_glyph(point, glyphs[1], style);
        }
    }

    // Top Right
    if (auto point = rect.top_right().translated({ -1, 0 }); in_bounds(point)) {
        m_io_terminal.put_glyph(point, glyphs[2], style);
    }

    // Right Edge
    for (auto y = rect.top() + 1; y < rect.bottom() - 1; y++) {
        auto point = Point { rect.right() - 1, y };
        if (in_bounds(point)) {
            m_io_terminal.put_glyph(point, glyphs[3], style);
        }
    }

    // Bottom Right
    if (auto point = rect.bottom_right().translated({ -1, -1 }); in_bounds(point)) {
        m_io_terminal.put_glyph(point, glyphs[4], style);
    }

    // Bottom Edge
    for (auto x = rect.left() + 1; x < rect.right() - 1; x++) {
        auto point = Point { x, rect.bottom() - 1 };
        if (in_bounds(point)) {
            m_io_terminal.put_glyph(point, glyphs[5], style);
        }
    }

    // Bottom Left
    if (auto point = rect.bottom_left().translated({ 0, -1 }); in_bounds(point)) {
        m_io_terminal.put_glyph(point, glyphs[6], style);
    }

    // Right Edge
    for (auto y = rect.top() + 1; y < rect.bottom() - 1; y++) {
        auto point = Point { rect.left(), y };
        if (in_bounds(point)) {
            m_io_terminal.put_glyph(point, glyphs[7], style);
        }
    }
}

void TerminalRenderer::clear_rect(const Rect& rect_in, Maybe<Color> color) {
    auto style = TerminalTextStyle { .foreground = {}, .background = color, .bold = false };

    for (auto& dirty_rect : m_dirty_rects) {
        auto rect = m_clip_rect.intersection_with(translate(rect_in)).intersection_with(dirty_rect);
        for (auto x = rect.left(); x < rect.right(); x++) {
            for (auto y = rect.top(); y < rect.bottom(); y++) {
                m_io_terminal.put_glyph({ x, y }, TerminalGlyph { " ", 1 }, style);
            }
        }
    }
}

void TerminalRenderer::put_glyph(const Point& position_in, const TerminalGlyph& glyph, const TerminalTextStyle& style) {
    auto position = translate(position_in);
    if (in_bounds(position)) {
        m_io_terminal.put_glyph(position, glyph, style);
    }
}

void TerminalRenderer::render_text(const Rect& rect, const StringView& text, const TerminalTextStyle& style, TextAlign alignment) {
    return render_complex_styled_text(
        rect, text,
        [&](auto) {
            return style;
        },
        alignment);
}

void TerminalRenderer::render_complex_styled_text(const Rect& rect_in, const StringView& text,
                                                  Function<TerminalTextStyle(size_t)> style_generator, TextAlign alignment) {
    if (rect_in.height() == 0 || rect_in.width() == 0) {
        return;
    }

    auto glyphs = convert_to_glyphs(text);
    auto text_width = glyphs.total_width();

    auto rect = translate(rect_in);
    auto start_position = [&] {
        auto y_position = [&] {
            switch (alignment) {
                case TextAlign::TopLeft:
                case TextAlign::TopCenter:
                case TextAlign::TopRight:
                    return rect.top();
                case TextAlign::CenterLeft:
                case TextAlign::Center:
                case TextAlign::CenterRight:
                    return rect.center().y();
                case TextAlign::BottomLeft:
                case TextAlign::BottomCenter:
                case TextAlign::BottomRight:
                    return rect.bottom() - 1;
                default:
                    return 0;
            }
        }();

        auto x_position = [&] {
            switch (alignment) {
                case TextAlign::TopLeft:
                case TextAlign::CenterLeft:
                case TextAlign::BottomLeft:
                    return rect.left();
                case TextAlign::TopCenter:
                case TextAlign::Center:
                case TextAlign::BottomCenter:
                    return rect.center().x() - (text_width / 2);
                case TextAlign::TopRight:
                case TextAlign::CenterRight:
                case TextAlign::BottomRight:
                    return rect.right() - text_width;
                default:
                    return 0;
            }
        }();
        return Point { x_position, y_position };
    }();

    auto x = start_position.x();
    auto text_index = 0;
    for (int i = 0; i < glyphs.size(); i++) {
        auto position = Point { x, start_position.y() };
        if (in_bounds(position)) {
            m_io_terminal.put_glyph(position, glyphs[i], style_generator(text_index));
        }
        x += glyphs[i].width();
        text_index += glyphs[i].text().size();
    }
}
}
