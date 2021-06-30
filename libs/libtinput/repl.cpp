#include <edit/suggestions.h>
#include <tinput/input_source.h>
#include <tinput/repl.h>

namespace TInput {
Repl::Repl(UniquePtr<History> history) : m_history(move(history)) {}

Repl::~Repl() {}

void Repl::enter(InputSource& input_source) {
    did_begin_input();

    while (!force_stop_input()) {
        did_begin_loop_iteration();

        auto result = input_source.get_input();
        if (result == InputResult::Error || result == InputResult::Eof) {
            break;
        }

        did_get_input(input_source.input_text());
    }

    did_end_input();
}

String Repl::get_main_prompt() const {
    return "> ";
}

String Repl::get_secondary_prompt() const {
    return "> ";
}

Edit::Suggestions Repl::get_suggestions(const String&, size_t) const {
    return {};
}
}
