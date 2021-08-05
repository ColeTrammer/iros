#include <liim/string_view.h>
#include <tinput/io_terminal.h>
#include <tinput/terminal_glyph.h>
#include <tinput/terminal_renderer.h>

namespace TInput {
void TerminalRenderer::clear_rect(const Rect& rect_in, Maybe<Color> color) {
    auto style = TerminalTextStyle { .foreground = {}, .background = color, .bold = false };

    auto rect = constrained(translate(rect_in));
    for (auto x = rect.left(); x < rect.right(); x++) {
        for (auto y = rect.top(); y < rect.bottom(); y++) {
            m_io_terminal.put_glyph({ x, y }, TerminalGlyph { " ", 1 }, style);
        }
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
