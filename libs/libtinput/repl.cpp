#include <edit/document.h>
#include <errno.h>
#include <tinput/repl.h>
#include <unistd.h>

#include "repl_panel.h"

namespace TInput {

Repl::Repl(UniquePtr<History> history) : m_history(move(history)) {}

Repl::~Repl() {}

InputResult Repl::get_input() {
    m_input.clear();

    for (;;) {
        ReplPanel panel(*this);
        auto document = Document::create_single_line(panel);
        document->set_type(get_input_type());
        document->set_auto_complete_mode(AutoCompleteMode::Always);
        document->set_preview_auto_complete(true);
        panel.set_document(move(document));
        panel.enter();

        if (panel.quit_by_eof()) {
            return InputResult::Eof;
        }

        if (panel.quit_by_interrupt()) {
            continue;
        }

        auto input_text = panel.document()->content_string();
        history().add(input_text);

        m_input = move(input_text);
        break;
    }

    return InputResult::Success;
}

String Repl::get_main_prompt() const {
    return "> ";
}

String Repl::get_secondary_prompt() const {
    return "> ";
}

Suggestions Repl::get_suggestions(const String &, size_t) const {
    return {};
}

}
