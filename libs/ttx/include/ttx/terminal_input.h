#pragma once

#include "di/container/string/string_view.h"
#include "di/container/vector/vector.h"
#include "di/vocab/variant/variant.h"
#include "ttx/key_event.h"

namespace ttx {
using Event = di::Variant<KeyEvent>;

class TerminalInputParser {
    enum class State {
        Base,
        Escape,
        CSI,
        SS3,
    };

public:
    auto parse(di::StringView input) -> di::Vector<Event>;

private:
    void handle_code_point(c32 input);
    void handle_base(c32 input);
    void handle_escape(c32 input);
    void handle_csi(c32 input);
    void handle_ss3(c32 input);

    void emit(KeyEvent&& event) { m_pending_events.push_back(di::move(event)); }

    State m_state { State::Base };
    di::String m_accumulator;
    di::Vector<Event> m_pending_events;
};
}
