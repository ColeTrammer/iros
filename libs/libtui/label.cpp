#include <tinput/terminal_glyph.h>
#include <tinput/terminal_renderer.h>
#include <tui/label.h>

namespace TUI {
void Label::render() {
    auto renderer = get_renderer();
    renderer.render_text(sized_rect(), m_text.view(), m_text_style, m_text_align);

    Panel::render();
}

void Label::set_text_align(TextAlign align) {
    if (m_text_align == align) {
        return;
    }

    m_text_align = align;
    invalidate();
}

void Label::set_terminal_text_style(const TInput::TerminalTextStyle& style) {
    if (m_text_style == style) {
        return;
    }

    m_text_style = style;
    invalidate();
}

void Label::set_shrink_to_fit(bool b) {
    if (m_shrink_to_fit == b) {
        return;
    }

    m_shrink_to_fit = b;
    if (m_shrink_to_fit) {
        set_layout_constraint({ TInput::convert_to_glyphs(m_text.view()).total_width(), LayoutConstraint::AutoSize });
    } else {
        set_layout_constraint({ LayoutConstraint::AutoSize, LayoutConstraint::AutoSize });
    }
}
}
