#include <edit/document.h>
#include <repl/repl_base.h>
#include <repl/terminal_input_source.h>

#include "repl_display.h"

namespace Repl {
TerminalInputSource::TerminalInputSource(ReplBase& repl) : InputSource(repl) {}

TerminalInputSource::~TerminalInputSource() {}

InputResult TerminalInputSource::get_input() {
    clear_input();

    for (;;) {
        ReplDisplay display(repl());
        auto document = Edit::Document::create_single_line();
        display.set_document(document);
        document->set_type(repl().get_input_type());
        document->set_auto_complete_mode(Edit::AutoCompleteMode::Always);
        document->set_preview_auto_complete(true);
        document->set_word_wrap_enabled(true);
        display.enter();

        if (display.quit_by_eof()) {
            return InputResult::Eof;
        }

        if (display.quit_by_interrupt()) {
            continue;
        }

        auto input_text = display.document()->content_string();
        repl().history().add(input_text);

        set_input(move(input_text));
        break;
    }

    return InputResult::Success;
}
}
