#include <edit/suggestions.h>
#include <repl/input_source.h>
#include <repl/repl_base.h>

namespace Repl {
ReplBase::ReplBase(UniquePtr<History> history) : m_history(move(history)) {}

ReplBase::~ReplBase() {}

void ReplBase::enter(InputSource& input_source) {
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

String ReplBase::get_main_prompt() const {
    return "> ";
}

String ReplBase::get_secondary_prompt() const {
    return "> ";
}

Edit::Suggestions ReplBase::get_suggestions(const String&, size_t) const {
    return {};
}
}
