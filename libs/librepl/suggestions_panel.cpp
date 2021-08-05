#include <edit/document.h>
#include <eventloop/event.h>
#include <tinput/terminal_renderer.h>

#include "repl_display.h"
#include "suggestions_panel.h"

namespace Repl {
constexpr int max_visible_suggestions = 5;

SuggestionsPanel::SuggestionsPanel(ReplDisplay& display, const Edit::Suggestions& suggestions)
    : m_display(display), m_suggestions(suggestions) {
    set_layout_constraint({ TUI::LayoutConstraint::AutoSize, max_visible_suggestions });
}

SuggestionsPanel::~SuggestionsPanel() {}

void SuggestionsPanel::render() {
    auto renderer = get_renderer();
    assert(m_suggestions.suggestion_count() > 0);
    for (int i = m_suggestion_offset; i < m_suggestion_offset + max_visible_suggestions && i < m_suggestions.suggestion_count(); i++) {
        auto& suggestion = m_suggestions.suggestion_list()[i];
        renderer.render_text(sized_rect().with_y(i - m_suggestion_offset).with_height(1), suggestion.view(),
                             { .foreground = {}, .background = {}, .bold = i == m_suggestion_index, .invert = false });
    }
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
            return m_display.exit_suggestion_panel();
        case App::Key::UpArrow:
            return prev_suggestion();
        case App::Key::DownArrow:
            return next_suggestion();
        case App::Key::Tab:
            return event.shift_down() ? prev_suggestion() : next_suggestion();
        case App::Key::Enter:
            return m_display.complete_suggestion(m_suggestions, m_suggestion_index);
        default:
            break;
    }
    return Panel::on_key_event(event);
}
}
