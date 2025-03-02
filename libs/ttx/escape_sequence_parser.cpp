#include "ttx/escape_sequence_parser.h"

#include "di/parser/prelude.h"
#include "di/util/scope_exit.h"

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
    // NOTE: this is modified from the reference to include ':' in addition to ';'.
    return (code_point >= 0x30 && code_point <= 0x39) || (code_point == 0x3B) || (code_point == 0x3A);
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
        execute(code_point);
        return;
    }

    if (is_printable(code_point)) {
        print(code_point);
        return;
    }
}

STATE(escape) {
    ON_ENTRY(Escape) {
        clear();
    }

    if (is_executable(code_point)) {
        execute(code_point);
        return;
    }

    if (code_point == 0x5B) {
        transition(State::CsiEntry);
        return;
    }

    if (m_mode == Mode::Input && code_point == 0x4F) {
        transition(State::Ss3);
        return;
    }

    // For the purposes of parsing input, any other code point should be treated
    // as if we were in the ground state. This allows us to recognize alt+key when
    // using the legacy mode.
    if (m_mode == Mode::Input) {
        execute(code_point);
        return;
    }

    if (is_escape_terminator(code_point)) {
        esc_dispatch(code_point);
        transition(State::Ground);
        return;
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        transition(State::EscapeIntermediate);
        return;
    }

    if (code_point == 0x58 || code_point == 0x5E || code_point == 0x5F) {
        transition(State::SosPmApcString);
        return;
    }

    if (code_point == 0x5D) {
        transition(State::OscString);
        return;
    }

    if (code_point == 0x50) {
        transition(State::DcsEntry);
        return;
    }

    if (code_point == 0x7F) {
        ignore(code_point);
        return;
    }
}

STATE(escape_intermediate) {
    ON_ENTRY_NOOP(EscapeIntermediate);

    if (is_executable(code_point)) {
        execute(code_point);
        return;
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        return;
    }

    if (code_point >= 0x30 && code_point <= 0x7E) {
        esc_dispatch(code_point);
        transition(State::Ground);
        return;
    }

    if (code_point == 0x7F) {
        ignore(code_point);
        return;
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
        transition(State::Ground);
        return;
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        transition(State::CsiIntermediate);
        return;
    }

    if (is_param(code_point)) {
        param(code_point);
        transition(State::CsiParam);
        return;
    }

    if (code_point >= 0x3C && code_point <= 0x3F) {
        collect(code_point);
        transition(State::CsiParam);
        return;
    }

    if (code_point == 0x7F) {
        ignore(code_point);
        return;
    }
}

STATE(csi_intermediate) {
    ON_ENTRY_NOOP(CsiIntermediate);

    if (is_executable(code_point)) {
        execute(code_point);
        return;
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        return;
    }

    if (is_csi_terminator(code_point)) {
        csi_dispatch(code_point);
        transition(State::Ground);
        return;
    }

    if (code_point >= 0x30 && code_point <= 0x3F) {
        transition(State::CsiIgnore);
        return;
    }

    if (code_point == 0x7F) {
        ignore(code_point);
        return;
    }
}

STATE(csi_param) {
    ON_ENTRY(CsiParam) {
        m_on_state_exit = [this] {
            if (!m_current_param.empty()) {
                add_param(di::parse<u32>(m_current_param).optional_value());
            }
        };
    }

    if (is_executable(code_point)) {
        execute(code_point);
        return;
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        transition(State::CsiIntermediate);
        return;
    }

    if (is_csi_terminator(code_point)) {
        transition(State::Ground);
        csi_dispatch(code_point);
        return;
    }

    if (is_param(code_point)) {
        param(code_point);
        return;
    }

    if (code_point >= 0x3C && code_point <= 0x3F) {
        transition(State::CsiIgnore);
        return;
    }

    if (code_point == 0x7F) {
        ignore(code_point);
        return;
    }
}

STATE(csi_ignore) {
    ON_ENTRY_NOOP(CsiIgnore);

    if (is_executable(code_point)) {
        execute(code_point);
        return;
    }

    if (is_csi_terminator(code_point)) {
        transition(State::Ground);
        return;
    }

    if ((code_point >= 0x20 && code_point <= 0x3F) || (code_point == 0x7F)) {
        ignore(code_point);
        return;
    }
}

STATE(dcs_entry) {
    ON_ENTRY(DcsEntry) {
        clear();
    }

    if (is_executable(code_point)) {
        ignore(code_point);
        return;
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        transition(State::DcsIntermediate);
        return;
    }

    if (is_param(code_point)) {
        param(code_point);
        transition(State::DcsParam);
        return;
    }

    if (code_point >= 0x3C && code_point <= 0x3F) {
        collect(code_point);
        transition(State::DcsParam);
        return;
    }

    if (is_dcs_terminator(code_point)) {
        transition(State::DcsPassthrough);
        return;
    }

    if (code_point == 0x7F) {
        ignore(code_point);
        return;
    }
}

STATE(dcs_param) {
    ON_ENTRY(DcsParam) {
        m_on_state_exit = [this] {
            if (!m_current_param.empty()) {
                add_param(di::parse<u32>(m_current_param).optional_value());
            }
        };
    }

    if (is_executable(code_point)) {
        ignore(code_point);
        return;
    }

    if (is_param(code_point)) {
        param(code_point);
        return;
    }

    if ((code_point >= 0x3C && code_point <= 0x3F)) {
        transition(State::DcsIgnore);
        return;
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        transition(State::DcsIntermediate);
        return;
    }

    if (is_dcs_terminator(code_point)) {
        transition(State::DcsPassthrough);
        return;
    }

    if (code_point == 0x7F) {
        ignore(code_point);
        return;
    }
}

STATE(dcs_intermediate) {
    ON_ENTRY_NOOP(DcsIntermediate);

    if (is_executable(code_point)) {
        ignore(code_point);
        return;
    }

    if (code_point >= 0x30 && code_point <= 0x3F) {
        transition(State::DcsIgnore);
        return;
    }

    if (is_intermediate(code_point)) {
        collect(code_point);
        return;
    }

    if (is_dcs_terminator(code_point)) {
        m_intermediate.push_back(code_point);
        transition(State::DcsPassthrough);
        return;
    }

    if (code_point == 0x7F) {
        ignore(code_point);
        return;
    }
}

STATE(dcs_passthrough) {
    ON_ENTRY(DcsPassthrough) {
        hook();
    }

    if (is_string_terminator(code_point)) {
        transition(State::Ground);
        return;
    }

    if (code_point == 0x7F) {
        ignore(code_point);
        return;
    }

    put(code_point);
}

STATE(dcs_ignore) {
    ON_ENTRY_NOOP(DcsIgnore);

    if (is_string_terminator(code_point)) {
        transition(State::Ground);
        return;
    }

    ignore(code_point);
}

STATE(osc_string) {
    ON_ENTRY(OscString) {
        osc_start();
    }

    if (is_string_terminator(code_point)) {
        m_saw_legacy_string_terminator = true;
        transition(State::Ground);
        return;
    }

    if (is_executable(code_point)) {
        ignore(code_point);
        return;
    }

    if (is_printable(code_point)) {
        osc_put(code_point);
        return;
    }
}

STATE(sos_pm_apc_string) {
    ON_ENTRY_NOOP(SosPmApcString);

    if (is_string_terminator(code_point)) {
        transition(State::Ground);
        return;
    }

    ignore(code_point);
}

STATE(ss3) {
    ON_ENTRY_NOOP(Ss3);

    output_ss3(code_point);
    transition(State::Ground);
}

void EscapeSequenceParser::ignore(c32) {}

void EscapeSequenceParser::print(c32 code_point) {
    m_result.push_back(PrintableCharacter(code_point));
}

void EscapeSequenceParser::execute(c32 code_point) {
    m_result.push_back(ControlCharacter(code_point, m_next_state == State::Escape));
    if (m_mode == Mode::Input) {
        transition(State::Ground);
    }
}

void EscapeSequenceParser::clear() {
    m_current_param.clear();
    m_params = {};
    m_last_separator_was_colon = false;
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

    auto _ = di::ScopeExit([&] {
        m_last_separator_was_colon = code_point == ':';
    });

    if (m_current_param.empty()) {
        add_param({});
        return;
    }

    add_param(di::parse<u32>(m_current_param.view()).optional_value());
    m_current_param.clear();
}

void EscapeSequenceParser::esc_dispatch(c32 code_point) {
    // Ignore string terminators (ESC \). These terminate OSC and DCS sequences,
    // but the current state machine exits these states immediately upon hitting
    // the ESC. Dispatching the string terminator should not happen.
    if (code_point == '\\') {
        return;
    }
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

void EscapeSequenceParser::put(c32 code_point) {
    m_data.push_back(code_point);
}

void EscapeSequenceParser::unhook() {
    m_result.push_back(DCS(di::move(m_intermediate), di::move(m_params), di::move(m_data)));
}

void EscapeSequenceParser::osc_start() {
    m_saw_legacy_string_terminator = false;
    m_on_state_exit = [this] {
        osc_end();
    };
}

void EscapeSequenceParser::osc_put(c32 code_point) {
    m_data.push_back(code_point);
}

void EscapeSequenceParser::osc_end() {
    auto terminator = m_saw_legacy_string_terminator ? "\a"_sv : "\033\\"_sv;
    m_result.push_back(OSC(di::move(m_data), terminator));
}

void EscapeSequenceParser::output_ss3(c32 code_point) {
    // SS3 A gets mapped to CSI A, with no intermediate or parameters. This
    // makes parsing key presses easier as we only worry about CSI.
    m_result.push_back(CSI({}, {}, code_point));
}

void EscapeSequenceParser::add_param(di::Optional<u32> param) {
    if (m_last_separator_was_colon) {
        if (param) {
            m_params.add_subparam(param.value());
        } else {
            m_params.add_empty_subparam();
        }
    } else {
        if (param) {
            m_params.add_param(param.value());
        } else {
            m_params.add_empty_param();
        }
    }
    m_last_separator_was_colon = false;
}

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
        transition(State::Ground);
        return;
    }

    if (code_point == 0x1B) {
        // When parsing input, recnogize ESC ESC as a key press.
        if (m_mode == Mode::Input && m_next_state == State::Escape) {
            execute(code_point);
            transition(State::Ground);
            return;
        }
        transition(State::Escape);
        return;
    }

    switch (m_next_state) {
#define __ENUMERATE_STATE(N, n)       \
    case State::N:                    \
        return n##_state(code_point);
        __ENUMERATE_STATES(__ENUMERATE_STATE)
#undef __ENUMERATE_STATE
    }
}

auto EscapeSequenceParser::parse_application_escape_sequences(di::StringView data) -> di::Vector<ParserResult> {
    m_mode = Mode::Application;
    for (auto code_point : data) {
        on_input(code_point);
    }

    return di::move(m_result);
}

auto EscapeSequenceParser::parse_input_escape_sequences(di::StringView data, bool flush) -> di::Vector<ParserResult> {
    m_mode = Mode::Input;
    for (auto code_point : data) {
        on_input(code_point);
    }

    // Special case: if we get a lone "escape" byte followed by no text,
    // we assume the user hit the escape key, and not that the terminal
    // partially transmitted an escape sequence. This really should be
    // using a timeout mechanism, and be completely disabled if the terminal
    // supports the kitty key protocol.
    if (flush && m_next_state == State::Escape) {
        transition(State::Ground);
        m_result.push_back(ControlCharacter('\x1b'));
    }

    return di::move(m_result);
}
}
