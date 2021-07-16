#include <edit/document.h>
#include <tinput/repl.h>
#include <tinput/terminal_input_source.h>

#include "repl_panel.h"

namespace TInput {
TerminalInputSource::TerminalInputSource(Repl& repl) : InputSource(repl) {}

TerminalInputSource::~TerminalInputSource() {}

InputResult TerminalInputSource::get_input() {
    clear_input();

    for (;;) {
        ReplPanel panel(repl());
        auto document = Edit::Document::create_single_line(panel);
        panel.set_document(document);
        document->set_type(repl().get_input_type());
        document->set_auto_complete_mode(Edit::AutoCompleteMode::Always);
        document->set_preview_auto_complete(true);
        document->set_word_wrap_enabled(true);
        panel.enter();

        if (panel.quit_by_eof()) {
            return InputResult::Eof;
        }

        if (panel.quit_by_interrupt()) {
            continue;
        }

        auto input_text = panel.document()->content_string();
        repl().history().add(input_text);

        set_input(move(input_text));
        break;
    }

    return InputResult::Success;
}
}
