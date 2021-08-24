#include <edit/document.h>
#include <repl/repl_base.h>
#include <repl/terminal_input_source.h>
#include <tinput/io_terminal.h>
#include <tui/application.h>
#include <tui/flex_layout_engine.h>

#include "repl_display.h"

namespace Repl {
class ReplLayoutEngine final : public TUI::LayoutEngine {
public:
    ReplLayoutEngine(TUI::Panel& parent) : TUI::LayoutEngine(parent) {}

    virtual void do_add(TUI::Panel& panel) override {
        assert(!m_display);
        m_display = static_cast<ReplDisplay*>(&panel);
    }

    virtual void layout() override {
        if (!m_display) {
            return;
        }

        auto inital_cursor_position = TUI::Application::the().io_terminal().initial_cursor_position();
        auto terminal_rect = TUI::Application::the().sized_rect();
        if (m_first_layout) {
            m_first_layout = false;
            auto y = inital_cursor_position.x() != 0 ? inital_cursor_position.y() + 1 : inital_cursor_position.y();
            m_display->set_positioned_rect({ 0, y, terminal_rect.width(), terminal_rect.height() - y });
            return;
        }

        // Invalidate the display, so that it will update itself based on the new terminal's dimensions.
        m_display->invalidate();
    }

private:
    ReplDisplay* m_display { nullptr };
    bool m_first_layout { true };
};

TerminalInputSource::TerminalInputSource(ReplBase& repl) : InputSource(repl) {}

TerminalInputSource::~TerminalInputSource() {}

InputResult TerminalInputSource::get_input() {
    clear_input();

    for (;;) {
        auto app = TUI::Application::try_create();
        if (!app) {
            return InputResult::Error;
        }

        auto& layout = app->set_layout_engine<ReplLayoutEngine>();

        auto& display = layout.add<ReplDisplay>(repl());
        display.set_auto_complete_mode(Edit::AutoCompleteMode::Always);
        display.set_preview_auto_complete(true);
        display.set_word_wrap_enabled(true);
        display.enter();

        auto document = Edit::Document::create_single_line();
        document->set_type(repl().get_input_type());
        display.set_document(document);
        app->enter();

        if (display.quit_by_eof()) {
            return InputResult::Eof;
        }

        if (display.quit_by_interrupt()) {
            continue;
        }

        auto input_text = display.document()->content_string();
        repl().history().add(input_text);

        set_input(move(input_text));
        return InputResult::Success;
    }
}
}
