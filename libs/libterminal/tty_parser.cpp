#include <terminal/tty_parser.h>

// #define TTY_PARSER_DEBUG

#define STATE(state) void TTYParser::state##_state([[maybe_unused]] uint8_t byte)

#define ON_ENTRY_NOOP(state)         \
    do {                             \
        m_last_state = State::state; \
    } while (0)

#define ON_ENTRY(state)                                   \
    bool __did_transition = State::state != m_last_state; \
    m_last_state = State::state;                          \
    if (__did_transition)

static inline bool is_printable(uint8_t byte) {
    return (byte >= 0x20 && byte <= 0x7F) || (byte >= 0xA0);
}

static inline bool is_executable(uint8_t byte) {
    return byte <= 0x17 || byte == 0x19 || (byte >= 0x1C && byte <= 0x1F);
}

static inline bool is_csi_terminator(uint8_t byte) {
    return byte >= 0x40 && byte <= 0x7E;
}

static inline bool is_param(uint8_t byte) {
    return (byte >= 0x30 && byte <= 0x39) || (byte == 0x3B);
}

static inline bool is_intermediate(uint8_t byte) {
    return byte >= 0x20 && byte <= 0x2F;
}

static inline bool is_string_terminator(uint8_t byte) {
    // NOTE: this is xterm specific.
    return byte == '\a';
}

static inline bool is_dcs_terminator(uint8_t byte) {
    return byte >= 0x40 && byte <= 0x7E;
}

static inline bool is_escape_terminator(uint8_t byte) {
    return (byte >= 0x30 && byte <= 0x4F) || (byte >= 0x51 && byte <= 0x57) || (byte == 0x59) || (byte == 0x5A) || (byte == 0x5C) ||
           (byte >= 0x60 && byte <= 0x7E);
}

TTYParser::TTYParser(TTYParserDispatcher& dispatcher) : m_dispatcher(dispatcher) {}

STATE(ground) {
    ON_ENTRY_NOOP(Ground);

    if (is_executable(byte)) {
        return execute(byte);
    }

    if (is_printable(byte)) {
        return print(byte);
    }
}

STATE(escape) {
    ON_ENTRY(Escape) { clear(); }

    if (is_executable(byte)) {
        return execute(byte);
    }

    if (is_escape_terminator(byte)) {
        esc_dispatch(byte);
        return transition(State::Ground);
    }

    if (is_intermediate(byte)) {
        collect(byte);
        return transition(State::EscapeIntermediate);
    }

    if (byte == 0x58 || byte == 0x5E || byte == 0x5F) {
        return transition(State::SosPmApcString);
    }

    if (byte == 0x5B) {
        return transition(State::CsiEntry);
    }

    if (byte == 0x5D) {
        return transition(State::OscString);
    }

    if (byte == 0x50) {
        return transition(State::DcsEntry);
    }

    if (byte == 0x7F) {
        return ignore(byte);
    }
}

STATE(escape_intermediate) {
    ON_ENTRY_NOOP(EscapeIntermediate);

    if (is_executable(byte)) {
        return execute(byte);
    }

    if (is_intermediate(byte)) {
        return collect(byte);
    }

    if (byte >= 0x30 && byte <= 0x7E) {
        esc_dispatch(byte);
        return transition(State::Ground);
    }

    if (byte == 0x7F) {
        return ignore(byte);
    }
}

STATE(csi_entry) {
    ON_ENTRY(CsiEntry) { clear(); }

    if (is_executable(byte)) {
        execute(byte);
        return;
    }

    if (is_csi_terminator(byte)) {
        csi_dispatch(byte);
        return transition(State::Ground);
    }

    if (is_intermediate(byte)) {
        collect(byte);
        return transition(State::CsiIntermediate);
    }

    if (is_param(byte)) {
        param(byte);
        return transition(State::CsiParam);
    }

    if (byte >= 0x3C && byte <= 0x3F) {
        collect(byte);
        return transition(State::CsiParam);
    }

    if (byte == 0x3A) {
        return transition(State::CsiIgnore);
    }

    if (byte == 0x7F) {
        return ignore(byte);
    }
}

STATE(csi_intermediate) {
    ON_ENTRY_NOOP(CsiIntermediate);

    if (is_executable(byte)) {
        return execute(byte);
    }

    if (is_intermediate(byte)) {
        return collect(byte);
    }

    if (is_csi_terminator(byte)) {
        csi_dispatch(byte);
        return transition(State::Ground);
    }

    if (byte >= 0x30 && byte <= 0x3F) {
        return transition(State::CsiIgnore);
    }

    if (byte == 0x7F) {
        return ignore(byte);
    }
}

STATE(csi_param) {
    ON_ENTRY(CsiParam) {
        m_on_state_exit = [this] {
            if (!m_current_param.empty()) {
                m_params.add(atoi(m_current_param.string()));
            }
        };
    }

    if (is_executable(byte)) {
        return execute(byte);
    }

    if (is_intermediate(byte)) {
        collect(byte);
        return transition(State::CsiIntermediate);
    }

    if (is_csi_terminator(byte)) {
        transition(State::Ground);
        return csi_dispatch(byte);
    }

    if (is_param(byte)) {
        return param(byte);
    }

    if (byte == 0x3A || (byte >= 0x3C && byte <= 0x3F)) {
        return transition(State::CsiIgnore);
    }

    if (byte == 0x7F) {
        return ignore(byte);
    }
}

STATE(csi_ignore) {
    ON_ENTRY_NOOP(CsiIgnore);

    if (is_executable(byte)) {
        return execute(byte);
    }

    if (is_csi_terminator(byte)) {
        return transition(State::Ground);
    }

    if ((byte >= 0x20 && byte <= 0x3F) || (byte == 0x7F)) {
        return ignore(byte);
    }
}

STATE(dcs_entry) {
    ON_ENTRY(DcsEntry) { clear(); }

    if (is_executable(byte)) {
        return ignore(byte);
    }

    if (is_intermediate(byte)) {
        collect(byte);
        return transition(State::DcsIntermediate);
    }

    if (is_param(byte)) {
        param(byte);
        return transition(State::DcsParam);
    }

    if (byte >= 0x3C && byte <= 0x3F) {
        collect(byte);
        return transition(State::DcsParam);
    }

    if (byte == 0x3A) {
        return transition(State::DcsIgnore);
    }

    if (is_dcs_terminator(byte)) {
        return transition(State::DcsPassthrough);
    }

    if (byte == 0x7F) {
        return ignore(byte);
    }
}

STATE(dcs_param) {
    ON_ENTRY(DcsParam) {
        m_on_state_exit = [this] {
            if (!m_current_param.empty()) {
                m_params.add(atoi(m_current_param.string()));
            }
        };
    }

    if (is_executable(byte)) {
        return ignore(byte);
    }

    if (is_param(byte)) {
        return param(byte);
    }

    if ((byte == 0x3A) || (byte >= 0x3C && byte <= 0x3F)) {
        return transition(State::DcsIgnore);
    }

    if (is_intermediate(byte)) {
        collect(byte);
        return transition(State::DcsIntermediate);
    }

    if (is_dcs_terminator(byte)) {
        return transition(State::DcsPassthrough);
    }

    if (byte == 0x7F) {
        return ignore(byte);
    }
}

STATE(dcs_intermediate) {
    ON_ENTRY_NOOP(DcsIntermediate);

    if (is_executable(byte)) {
        return ignore(byte);
    }

    if (byte >= 0x30 && byte <= 0x3F) {
        return transition(State::DcsIgnore);
    }

    if (is_intermediate(byte)) {
        return collect(byte);
    }

    if (is_dcs_terminator(byte)) {
        return transition(State::DcsPassthrough);
    }

    if (byte == 0x7F) {
        return ignore(byte);
    }
}

STATE(dcs_passthrough) {
    ON_ENTRY(DcsPassthrough) { hook(); }

    if (is_string_terminator(byte)) {
        return transition(State::Ground);
    }

    if (byte == 0x7F) {
        return ignore(byte);
    }

    return put(byte);
}

STATE(dcs_ignore) {
    ON_ENTRY_NOOP(DcsIgnore);

    if (is_string_terminator(byte)) {
        return transition(State::Ground);
    }

    return ignore(byte);
}

STATE(osc_string) {
    ON_ENTRY(OscString) { osc_start(); }

    if (is_string_terminator(byte)) {
        return transition(State::Ground);
    }

    if (is_executable(byte)) {
        return ignore(byte);
    }

    if (is_printable(byte)) {
        return osc_put(byte);
    }
}

STATE(sos_pm_apc_string) {
    ON_ENTRY_NOOP(SosPmApcString);

    if (is_string_terminator(byte)) {
        return transition(State::Ground);
    }

    return ignore(byte);
}

void TTYParser::ignore(uint8_t) {}

void TTYParser::print(uint8_t byte) {
#ifdef TTY_PARSER_DEBUG
    fprintf(stderr, "PRINT '%c'\n", byte);
#endif /* TTY_PARSER_DEBUG */
    m_dispatcher.on_printable_character(byte);
}

void TTYParser::execute(uint8_t byte) {
#ifdef TTY_PARSER_DEBUG
    fprintf(stderr, "C0 %#X %#o %d\n", byte, byte, byte);
#endif /* TTY_PARSER_DEBUG */
    m_dispatcher.on_c0_character(byte);
}

void TTYParser::clear() {
    m_current_param.clear();
    m_params.clear();
    m_intermediate.clear();
}

void TTYParser::collect(uint8_t byte) {
    m_intermediate += String(byte);
}

void TTYParser::param(uint8_t byte) {
    if (byte != ';') {
        m_current_param += String(byte);
        return;
    }

    if (m_current_param.empty()) {
        m_params.add(0);
        return;
    }

    m_params.add(atoi(m_current_param.string()));
    m_current_param.clear();
}

void TTYParser::esc_dispatch(uint8_t terminator) {
#ifdef TTY_PARSER_DEBUG
    fprintf(stderr, "ESC %s %c\n", m_intermediate.string(), terminator);
#endif /* TTY_PARSER_DEBUG */
    m_dispatcher.on_escape(m_intermediate, terminator);
}

void TTYParser::csi_dispatch(uint8_t terminator) {
#ifdef TTY_PARSER_DEBUG
    fprintf(stderr, "CSI %s ", m_intermediate.string());
    for (auto param : m_params) {
        fprintf(stderr, "%d ", param);
    }
    fprintf(stderr, "%c\n", terminator);
#endif /* TTY_PARSER_DEBUG */
    m_dispatcher.on_csi(m_intermediate, m_params, terminator);
}

void TTYParser::hook() {
    m_on_state_exit = [this] {
        unhook();
    };
}

void TTYParser::put(uint8_t) {}

void TTYParser::unhook() {}

void TTYParser::osc_start() {
    m_on_state_exit = [this] {
        osc_end();
    };
}

void TTYParser::osc_put(uint8_t) {}

void TTYParser::osc_end() {}

void TTYParser::transition(State state) {
    m_on_state_exit.safe_call();
    m_on_state_exit = nullptr;
    m_next_state = state;
}

void TTYParser::on_input(uint8_t byte) {
    if (byte == 0x18 || byte == 0x1A) {
        execute(byte);
        return transition(State::Ground);
    }

    if (byte == 0x1B) {
        return transition(State::Escape);
    }

    switch (m_next_state) {
#define __ENUMERATE_STATE(N, n) \
    case State::N:              \
        return n##_state(byte);
        __ENUMERATE_STATES(__ENUMERATE_STATE)
#undef __ENUMERATE_STATE
    }
}

void TTYParser::parse(Span<const uint8_t> data) {
    for (auto byte : data) {
        on_input(byte);
    }
}
