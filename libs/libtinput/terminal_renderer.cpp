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
}
