#include <edit/document.h>
#include <eventloop/event.h>
#include <tinput/terminal_glyph.h>
#include <tinput/terminal_renderer.h>

#include "repl_display.h"
#include "suggestions_panel.h"

namespace Repl {
constexpr int max_visible_suggestions = 5;

SuggestionsPanel::SuggestionsPanel(ReplDisplay& display) : m_display(display) {
    set_layout_constraint({ TUI::LayoutConstraint::AutoSize, max_visible_suggestions });
}

SuggestionsPanel::~SuggestionsPanel() {}

void SuggestionsPanel::render() {
    auto renderer = get_renderer();
    auto suggestions_rendered = 0;
    for (int i = m_suggestion_offset; i < m_suggestion_offset + max_visible_suggestions && i < m_suggestions.suggestion_count();
         i++, suggestions_rendered++) {
        auto& suggestion = m_suggestions.suggestion_list()[i];
        auto text_width = TInput::convert_to_glyphs(suggestion.view()).total_width();
        renderer.render_text(sized_rect().with_y(i - m_suggestion_offset).with_height(1), suggestion.view(),
                             { .foreground = {}, .background = {}, .bold = i == m_suggestion_index, .invert = false });
        renderer.clear_rect({ text_width, i - m_suggestion_offset, sized_rect().width() - text_width, 1 });
    }

    renderer.clear_rect(sized_rect().with_y(suggestions_rendered).with_height(sized_rect().height() - suggestions_rendered));
}

void SuggestionsPanel::set_suggestions(const Edit::Suggestions& suggestions) {
    m_suggestions = suggestions;
    m_suggestion_index = 0;
    m_suggestion_offset = 0;
    invalidate();
}

void SuggestionsPanel::on_key_event(const App::KeyEvent& event) {
    auto next_suggestion = [this] {
        if (m_suggestion_index < m_suggestions.suggestion_count() - 1) {
            m_suggestion_index++;
            if (m_suggestion_index >= m_suggestion_offset + max_visible_suggestions) {
                m_suggestion_offset++;
            }
            invalidate();
        }
    };

    auto prev_suggestion = [this] {
        if (m_suggestion_index != 0) {
            m_suggestion_index--;
            if (m_suggestion_index < m_suggestion_offset) {
                m_suggestion_offset--;
            }
            invalidate();
        }
    };

    switch (event.key()) {
        case App::Key::Escape:
            return m_display.hide_suggestions_panel();
        case App::Key::UpArrow:
            return prev_suggestion();
        case App::Key::DownArrow:
            return next_suggestion();
        case App::Key::Tab:
            if (m_suggestions.suggestion_count() == 1) {
                return m_display.complete_suggestion(m_suggestions, 0);
            }
            return event.shift_down() ? prev_suggestion() : next_suggestion();
        case App::Key::Enter:
            return m_display.complete_suggestion(m_suggestions, m_suggestion_index);
        default:
            break;
    }
    return Panel::on_key_event(event);
}
}
