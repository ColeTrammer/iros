#include <liim/string_view.h>
#include <tinput/io_terminal.h>
#include <tinput/terminal_renderer.h>

namespace TInput {
void TerminalRenderer::clear_rect(const Rect& rect_in, Maybe<Color> color) {
    auto style = TerminalTextStyle { .foreground = {}, .background = color, .bold = false };

    auto rect = constrained(translate(rect_in));
    for (auto x = rect.left(); x < rect.right(); x++) {
        for (auto y = rect.top(); y < rect.bottom(); y++) {
            m_io_terminal.put_text({ x, y }, " "sv, style);
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

    // FIXME: this requires a more sophisticated computation.
    auto text_width = static_cast<int>(text.size());

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

    for (size_t i = 0; i < text.size(); i++) {
        auto position = Point { start_position.x() + static_cast<int>(i), start_position.y() };
        if (in_bounds(position)) {
            m_io_terminal.put_text(position, text.substring(i, 1), style_generator(i));
        }
    }
}
}
