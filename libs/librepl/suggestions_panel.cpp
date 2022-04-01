#include <edit/document.h>
#include <eventloop/event.h>
#include <tinput/terminal_glyph.h>
#include <tinput/terminal_renderer.h>

#include "repl_display.h"
#include "suggestions_panel.h"

namespace Repl {
constexpr int max_visible_suggestions = 5;

SuggestionsPanel::SuggestionsPanel(ReplDisplay& display) : m_display(display), m_suggestions(display.suggestions()) {}

void SuggestionsPanel::did_attach() {
    set_layout_constraint({ App::LayoutConstraint::AutoSize, max_visible_suggestions });

    on<App::KeyDownEvent>([this](const App::KeyDownEvent& event) {
        auto next_suggestion = [this] {
            if (m_suggestion_index < m_suggestions.size() - 1) {
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
                m_display.hide_suggestions_panel();
                break;
            case App::Key::UpArrow:
                prev_suggestion();
                break;
            case App::Key::DownArrow:
                next_suggestion();
                break;
            case App::Key::Tab:
            case App::Key::Enter:
                if (m_suggestions.empty()) {
                    m_display.hide_suggestions_panel();
                    break;
                }
                m_display.complete_suggestion(m_suggestions[m_suggestion_index]);
                break;
            default:
                return false;
        }
        return true;
    });

    Panel::did_attach();
}

SuggestionsPanel::~SuggestionsPanel() {}

void SuggestionsPanel::render() {
    auto renderer = get_renderer();
    auto suggestions_rendered = 0;
    for (int i = m_suggestion_offset; i < m_suggestion_offset + max_visible_suggestions && i < m_suggestions.size();
         i++, suggestions_rendered++) {
        auto& suggestion = m_suggestions[i];
        auto suggestion_rect = sized_rect().with_y(i - m_suggestion_offset).with_height(1);

        // Render the suggestion such that the characters actually matched are highlighted with a different color.
        // Other characters are rendered with the default color.
        auto match_index_start = 0;
        renderer.render_complex_styled_text(suggestion_rect, suggestion.content().view(), [&](size_t index) {
            auto bold = i == m_suggestion_index;
            auto part_of_match =
                match_index_start < suggestion.detailed_match().size() && suggestion.detailed_match()[match_index_start] == index;
            if (part_of_match) {
                match_index_start++;
            }
            return TInput::TerminalTextStyle {
                .foreground = part_of_match ? Maybe<Color> { VGA_COLOR_RED } : Maybe<Color> {},
                .background = {},
                .bold = bold,
                .invert = false,
            };
        });

        auto text_width = TInput::convert_to_glyphs(suggestion.content().view()).total_width();
        renderer.clear_rect({ text_width, i - m_suggestion_offset, sized_rect().width() - text_width, 1 });
    }

    renderer.clear_rect(sized_rect().with_y(suggestions_rendered).with_height(sized_rect().height() - suggestions_rendered));
}

void SuggestionsPanel::did_update_suggestions() {
    m_suggestion_index = 0;
    m_suggestion_offset = 0;
    invalidate();
}
}
