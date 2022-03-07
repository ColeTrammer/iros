#include <app/flex_layout_engine.h>
#include <edit/document.h>
#include <tui/label.h>

#include "terminal_display.h"
#include "terminal_search.h"

TerminalSearch::TerminalSearch(TerminalDisplay& host_display, String initial_text)
    : m_host_display(host_display), m_initial_text(move(initial_text)) {}

void TerminalSearch::did_attach() {
    auto& layout = set_layout_engine<App::VerticalFlexLayoutEngine>();
    layout.set_margins({ 1, 1, 1, 1 });

    auto& search_container = layout.add<TUI::Panel>();
    auto& replace_container = layout.add<TUI::Panel>();

    {
        auto& search_layout = search_container.set_layout_engine<App::HorizontalFlexLayoutEngine>();

        auto& search_label = search_layout.add<TUI::Label>("Find: ");
        search_label.set_shrink_to_fit(true);

        auto search_document = Edit::Document::create_from_text(m_initial_text);
        search_document->set_submittable(true);
        listen<Edit::Submit>(*search_document, [this](auto&) {
            m_host_display.move_cursor_to_next_search_match();
        });

        listen<Edit::Change>(*search_document, [this, document = search_document.get()](auto&) {
            auto to_find = document->content_string();
            m_host_display.set_search_text(move(to_find));
        });

        auto& search_text_box = search_layout.add<TerminalDisplay>();
        listen_intercept<App::KeyDownEvent>(search_text_box, [this, &replace_container](const App::KeyDownEvent& event) {
            if (event.key() == App::Key::Tab && event.modifiers() == 0) {
                replace_container.make_focused();
                return true;
            }
            if (event.key() == App::Key::Escape) {
                m_host_display.hide_search_panel();
                m_host_display.make_focused();
                return true;
            }
            return false;
        });
        search_text_box.set_document(search_document);
        search_text_box.enter();

        search_document->select_all(search_text_box.base(), search_text_box.main_cursor());

        search_container.set_focus_proxy(&search_text_box.base());
        set_focus_proxy(&search_text_box.base());
    }

    {
        auto& replace_layout = replace_container.set_layout_engine<App::HorizontalFlexLayoutEngine>();

        auto& replace_label = replace_layout.add<TUI::Label>("Replace: ");
        replace_label.set_shrink_to_fit(true);

        auto replace_document = Edit::Document::create_from_text("");
        replace_document->set_submittable(true);
        listen<Edit::Submit>(*replace_document, [this, document = replace_document.get()](auto&) {
            m_host_display.replace_next_search_match(document->content_string());
        });

        auto& replace_text_box = replace_layout.add<TerminalDisplay>();
        listen_intercept<App::KeyDownEvent>(replace_text_box, [this, &search_container](const App::KeyDownEvent& event) {
            if (event.key() == App::Key::Tab && event.modifiers() == App::KeyModifier::Shift) {
                search_container.make_focused();
                return true;
            }
            if (event.key() == App::Key::Escape) {
                m_host_display.hide_search_panel();
                m_host_display.make_focused();
                return true;
            }
            return false;
        });
        replace_text_box.set_document(replace_document);

        replace_container.set_focus_proxy(&replace_text_box.base());
    }

    Panel::did_attach();
}

TerminalSearch::~TerminalSearch() {}
