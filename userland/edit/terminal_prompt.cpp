#include <edit/document.h>
#include <tui/flex_layout_engine.h>
#include <tui/label.h>

#include "terminal_display.h"
#include "terminal_prompt.h"

TerminalPrompt::TerminalPrompt(TerminalDisplay& host_display, String prompt, String initial_value)
    : m_host_display(host_display), m_prompt(move(prompt)), m_initial_value(move(initial_value)) {}

void TerminalPrompt::initialize() {
    auto& layout = set_layout_engine<TUI::FlexLayoutEngine>(TUI::FlexLayoutEngine::Direction::Horizontal);
    layout.set_padding(1);

    auto& label = layout.add<TUI::Label>(m_prompt);
    label.set_shrink_to_fit(true);

    auto document = Edit::Document::create_from_text(m_initial_value);
    document->set_input_mode(Edit::InputMode::InputText);
    document->set_submittable(true);
    document->on_submit = [this, document] {
        // NOTE: We take reference to outselves since the on_submit hook will most probably remove the prompt from its tree.
        //       If a reference is not taken, then doing so will destruct ourselves, which also destroys this very callback.
        //       To prevent the callback from being destroyed while it is running, it is necessary to take a reference to ourselves.
        auto protector = shared_from_this();
        on_submit.safe_call(document->content_string());
    };
    document->on_escape_press = [this] {
        m_host_display.hide_prompt_panel();
    };

    m_display = layout.add<TerminalDisplay>().shared_from_this();
    m_display->set_steals_focus(true);
    m_display->set_document(move(document));
    m_display->enter();

    set_focus_proxy(m_display.get());
}

TerminalPrompt::~TerminalPrompt() {}
