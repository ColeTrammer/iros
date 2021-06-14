#include <edit/document.h>
#include <errno.h>
#include <tinput/repl.h>
#include <unistd.h>

#include "repl_panel.h"

namespace TInput {

Repl::~Repl() {}

InputResult Repl::get_input() {
    m_input.clear();

    for (;;) {
        ReplPanel panel(*this);
        auto document = Document::create_single_line(panel);
        document->set_type(get_input_type());
        panel.set_document(move(document));
        panel.enter();

        if (panel.quit_by_interrupt()) {
            continue;
        }

        if (panel.quit_by_eof()) {
            return InputResult::Eof;
        }

        m_input = panel.document()->content_string();
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

Vector<String> Repl::get_suggestions(const String &, size_t) const {
    return {};
}

}
