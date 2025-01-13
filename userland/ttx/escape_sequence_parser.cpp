#include "escape_sequence_parser.h"

#include "di/parser/prelude.h"

#define STATE(state) void EscapeSequenceParser::state##_state([[maybe_unused]] c32 code_point)

#define ON_ENTRY_NOOP(state)         \
    do {                             \
        m_last_state = State::state; \
    } while (0)

#define ON_ENTRY(state)                                   \
    auto __did_transition = State::state != m_last_state; \
    m_last_state = State::state;                          \
    if (__did_transition)

namespace ttx {
static inline auto is_printable(c32 code_point) -> bool {
    return (code_point >= 0x20 && code_point <= 0x7F) || (code_point >= 0xA0);
}

static inline auto is_executable(c32 code_point) -> bool {
    return code_point <= 0x17 || code_point == 0x19 || (code_point >= 0x1C && code_point <= 0x1F);
}

static inline auto is_csi_terminator(c32 code_point) -> bool {
    return code_point >= 0x40 && code_point <= 0x7E;
}

static inline auto is_param(c32 code_point) -> bool {
    return (code_point >= 0x30 && code_point <= 0x39) || (code_point == 0x3B);
}

static inline auto is_intermediate(c32 code_point) -> bool {
    return code_point >= 0x20 && code_point <= 0x2F;
}

static inline auto is_string_terminator(c32 code_point) -> bool {
    // NOTE: this is xterm specific.
    return code_point == '\a';
}

static inline auto is_dcs_terminator(c32 code_point) -> bool {
    return code_point >= 0x40 && code_point <= 0x7E;
}

static inline auto is_escape_terminator(c32 code_point) -> bool {
    return (code_point >= 0x30 && code_point <= 0x4F) || (code_point >= 0x51 && code_point <= 0x57) ||
           (code_point == 0x59) || (code_point == 0x5A) || (code_point == 0x5C) ||
           (code_point >= 0x60 && code_point <= 0x7E);
}

STATE(ground) {
    ON_ENTRY_NOOP(Ground);

    if (is_executable(code_point)) {
        return execute(code_point);
    }

    if (is_printable(code_point)) {
        return print(code_point);
    }
}

STATE(escape) {
    ON_ENTRY(Escape) {
        clear();
    }

    if (is_executable(code_point)) {
        return execute(code_point);
    }

    if (is_escape_terminator(code_point)) {
        esc_dispatch(code_point);
        return transition(State::Ground);
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        return transition(State::EscapeIntermediate);
    }

    if (code_point == 0x58 || code_point == 0x5E || code_point == 0x5F) {
        return transition(State::SosPmApcString);
    }

    if (code_point == 0x5B) {
        return transition(State::CsiEntry);
    }

    if (code_point == 0x5D) {
        return transition(State::OscString);
    }

    if (code_point == 0x50) {
        return transition(State::DcsEntry);
    }

    if (code_point == 0x7F) {
        return ignore(code_point);
    }
}

STATE(escape_intermediate) {
    ON_ENTRY_NOOP(EscapeIntermediate);

    if (is_executable(code_point)) {
        return execute(code_point);
    }

    if (is_intermediate(code_point)) {
        return collect(code_point);
    }

    if (code_point >= 0x30 && code_point <= 0x7E) {
        esc_dispatch(code_point);
        return transition(State::Ground);
    }

    if (code_point == 0x7F) {
        return ignore(code_point);
    }
}

STATE(csi_entry) {
    ON_ENTRY(CsiEntry) {
        clear();
    }

    if (is_executable(code_point)) {
        execute(code_point);
        return;
    }

    if (is_csi_terminator(code_point)) {
        csi_dispatch(code_point);
        return transition(State::Ground);
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        return transition(State::CsiIntermediate);
    }

    if (is_param(code_point)) {
        param(code_point);
        return transition(State::CsiParam);
    }

    if (code_point >= 0x3C && code_point <= 0x3F) {
        collect(code_point);
        return transition(State::CsiParam);
    }

    if (code_point == 0x3A) {
        return transition(State::CsiIgnore);
    }

    if (code_point == 0x7F) {
        return ignore(code_point);
    }
}

STATE(csi_intermediate) {
    ON_ENTRY_NOOP(CsiIntermediate);

    if (is_executable(code_point)) {
        return execute(code_point);
    }

    if (is_intermediate(code_point)) {
        return collect(code_point);
    }

    if (is_csi_terminator(code_point)) {
        csi_dispatch(code_point);
        return transition(State::Ground);
    }

    if (code_point >= 0x30 && code_point <= 0x3F) {
        return transition(State::CsiIgnore);
    }

    if (code_point == 0x7F) {
        return ignore(code_point);
    }
}

STATE(csi_param) {
    ON_ENTRY(CsiParam) {
        m_on_state_exit = [this] {
            if (!m_current_param.empty()) {
                m_params.push_back(di::parse<i32>(m_current_param).value_or(0));
            }
        };
    }

    if (is_executable(code_point)) {
        return execute(code_point);
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        return transition(State::CsiIntermediate);
    }

    if (is_csi_terminator(code_point)) {
        transition(State::Ground);
        return csi_dispatch(code_point);
    }

    if (is_param(code_point)) {
        return param(code_point);
    }

    if (code_point == 0x3A || (code_point >= 0x3C && code_point <= 0x3F)) {
        return transition(State::CsiIgnore);
    }

    if (code_point == 0x7F) {
        return ignore(code_point);
    }
}

STATE(csi_ignore) {
    ON_ENTRY_NOOP(CsiIgnore);

    if (is_executable(code_point)) {
        return execute(code_point);
    }

    if (is_csi_terminator(code_point)) {
        return transition(State::Ground);
    }

    if ((code_point >= 0x20 && code_point <= 0x3F) || (code_point == 0x7F)) {
        return ignore(code_point);
    }
}

STATE(dcs_entry) {
    ON_ENTRY(DcsEntry) {
        clear();
    }

    if (is_executable(code_point)) {
        return ignore(code_point);
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        return transition(State::DcsIntermediate);
    }

    if (is_param(code_point)) {
        param(code_point);
        return transition(State::DcsParam);
    }

    if (code_point >= 0x3C && code_point <= 0x3F) {
        collect(code_point);
        return transition(State::DcsParam);
    }

    if (code_point == 0x3A) {
        return transition(State::DcsIgnore);
    }

    if (is_dcs_terminator(code_point)) {
        return transition(State::DcsPassthrough);
    }

    if (code_point == 0x7F) {
        return ignore(code_point);
    }
}

STATE(dcs_param) {
    ON_ENTRY(DcsParam) {
        m_on_state_exit = [this] {
            if (!m_current_param.empty()) {
                m_params.push_back(di::parse<i32>(m_current_param).value_or(0));
            }
        };
    }

    if (is_executable(code_point)) {
        return ignore(code_point);
    }

    if (is_param(code_point)) {
        return param(code_point);
    }

    if ((code_point == 0x3A) || (code_point >= 0x3C && code_point <= 0x3F)) {
        return transition(State::DcsIgnore);
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        return transition(State::DcsIntermediate);
    }

    if (is_dcs_terminator(code_point)) {
        return transition(State::DcsPassthrough);
    }

    if (code_point == 0x7F) {
        return ignore(code_point);
    }
}

STATE(dcs_intermediate) {
    ON_ENTRY_NOOP(DcsIntermediate);

    if (is_executable(code_point)) {
        return ignore(code_point);
    }

    if (code_point >= 0x30 && code_point <= 0x3F) {
        return transition(State::DcsIgnore);
    }

    if (is_intermediate(code_point)) {
        return collect(code_point);
    }

    if (is_dcs_terminator(code_point)) {
        return transition(State::DcsPassthrough);
    }

    if (code_point == 0x7F) {
        return ignore(code_point);
    }
}

STATE(dcs_passthrough) {
    ON_ENTRY(DcsPassthrough) {
        hook();
    }

    if (is_string_terminator(code_point)) {
        return transition(State::Ground);
    }

    if (code_point == 0x7F) {
        return ignore(code_point);
    }

    return put(code_point);
}

STATE(dcs_ignore) {
    ON_ENTRY_NOOP(DcsIgnore);

    if (is_string_terminator(code_point)) {
        return transition(State::Ground);
    }

    return ignore(code_point);
}

STATE(osc_string) {
    ON_ENTRY(OscString) {
        osc_start();
    }

    if (is_string_terminator(code_point)) {
        return transition(State::Ground);
    }

    if (is_executable(code_point)) {
        return ignore(code_point);
    }

    if (is_printable(code_point)) {
        return osc_put(code_point);
    }
}

STATE(sos_pm_apc_string) {
    ON_ENTRY_NOOP(SosPmApcString);

    if (is_string_terminator(code_point)) {
        return transition(State::Ground);
    }

    return ignore(code_point);
}

void EscapeSequenceParser::ignore(c32) {}

void EscapeSequenceParser::print(c32 code_point) {
    m_result.push_back(PrintableCharacter(code_point));
}

void EscapeSequenceParser::execute(c32 code_point) {
    m_result.push_back(ControlCharacter(code_point));
}

void EscapeSequenceParser::clear() {
    m_current_param.clear();
    m_params.clear();
    m_intermediate.clear();
}

void EscapeSequenceParser::collect(c32 code_point) {
    m_intermediate.push_back(code_point);
}

void EscapeSequenceParser::param(c32 code_point) {
    if (code_point != ';' && code_point != ':') {
        m_current_param.push_back(code_point);
        return;
    }

    if (m_current_param.empty()) {
        m_params.push_back(0);
        return;
    }

    m_params.push_back(di::parse<i32>(m_current_param.view()).value_or(0));
    m_current_param.clear();
}

void EscapeSequenceParser::esc_dispatch(c32 code_point) {
    m_result.push_back(Escape(di::move(m_intermediate), code_point));
}

void EscapeSequenceParser::csi_dispatch(c32 code_point) {
    m_result.push_back(CSI(di::move(m_intermediate), di::move(m_params), code_point));
}

void EscapeSequenceParser::hook() {
    m_on_state_exit = [this] {
        unhook();
    };
}

void EscapeSequenceParser::put(c32) {}

void EscapeSequenceParser::unhook() {}

void EscapeSequenceParser::osc_start() {
    m_on_state_exit = [this] {
        osc_end();
    };
}

void EscapeSequenceParser::osc_put(c32) {}

void EscapeSequenceParser::osc_end() {}

void EscapeSequenceParser::transition(State state) {
    if (m_on_state_exit) {
        m_on_state_exit();
    }
    m_on_state_exit = nullptr;
    m_next_state = state;
}

void EscapeSequenceParser::on_input(c32 code_point) {
    if (code_point == 0x18 || code_point == 0x1A) {
        execute(code_point);
        return transition(State::Ground);
    }

    if (code_point == 0x1B) {
        return transition(State::Escape);
    }

    switch (m_next_state) {
#define __ENUMERATE_STATE(N, n)       \
    case State::N:                    \
        return n##_state(code_point);
        __ENUMERATE_STATES(__ENUMERATE_STATE)
#undef __ENUMERATE_STATE
    }
}

auto EscapeSequenceParser::parse(di::StringView data) -> di::Vector<ParserResult> {
    for (auto code_point : data) {
        on_input(code_point);
    }
    return di::move(m_result);
}
}
