#include <edit/document.h>
#include <tui/flex_layout_engine.h>
#include <tui/label.h>

#include "terminal_display.h"
#include "terminal_search.h"

TerminalSearch::TerminalSearch(TerminalDisplay& host_display, String initial_text)
    : m_host_display(host_display), m_initial_text(move(initial_text)) {}

void TerminalSearch::initialize() {
    auto& layout = set_layout_engine<TUI::FlexLayoutEngine>(TUI::FlexLayoutEngine::Direction::Horizontal);
    layout.set_padding(1);

    auto& label = layout.add<TUI::Label>("Find: ");
    label.set_shrink_to_fit(true);

    auto search_document = Edit::Document::create_from_text(m_initial_text);
    search_document->set_input_mode(Edit::InputMode::InputText);
    search_document->set_submittable(true);
    search_document->on_submit = [this] {
        m_host_display.document()->move_cursor_to_next_search_match(m_host_display, m_host_display.cursors().main_cursor());
    };
    search_document->on_change = [this, search_document] {
        auto to_find = search_document->content_string();
        m_host_display.document()->set_search_text(move(to_find));
    };
    search_document->on_escape_press = [this] {
        m_host_display.hide_search_panel();
    };

    auto& text_box = layout.add<TerminalDisplay>();
    text_box.set_document(search_document);
    text_box.enter();

    set_focus_proxy(&text_box);

    Panel::initialize();
}

TerminalSearch::~TerminalSearch() {}
