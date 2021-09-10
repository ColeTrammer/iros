#pragma once

#include <liim/function.h>
#include <liim/span.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <stdint.h>

class TTYParserDispatcher {
public:
    virtual ~TTYParserDispatcher() {}

    virtual void on_printable_character(uint8_t byte) = 0;
    virtual void on_csi(const String& intermediate, const Vector<int>& params, uint8_t terminator) = 0;
    virtual void on_escape(const String& intermediate, uint8_t terminator) = 0;
    virtual void on_c0_character(uint8_t byte) = 0;
};

class TTYParser {
public:
    TTYParser(TTYParserDispatcher& dispatcher);

    void parse(Span<const uint8_t> data);

private:
// VT500-Series parser states from https://vt100.net/emu/dec_ansi_parser
#define __ENUMERATE_STATES(M)                  \
    M(Ground, ground)                          \
    M(Escape, escape)                          \
    M(EscapeIntermediate, escape_intermediate) \
    M(CsiEntry, csi_entry)                     \
    M(CsiParam, csi_param)                     \
    M(CsiIntermediate, csi_intermediate)       \
    M(CsiIgnore, csi_ignore)                   \
    M(DcsEntry, dcs_entry)                     \
    M(DcsParam, dcs_param)                     \
    M(DcsIntermediate, dcs_intermediate)       \
    M(DcsPassthrough, dcs_passthrough)         \
    M(DcsIgnore, dcs_ignore)                   \
    M(OscString, osc_string)                   \
    M(SosPmApcString, sos_pm_apc_string)

    enum class State {
#define __ENUMERATE_STATE(N, n) N,
        __ENUMERATE_STATES(__ENUMERATE_STATE)
#undef __ENUMERATE_STATE
    };

#define __ENUMERATE_STATE(N, n) void n##_state(uint8_t byte);
    __ENUMERATE_STATES(__ENUMERATE_STATE)
#undef __ENUMERATE_STATE

    void ignore(uint8_t byte);
    void print(uint8_t byte);
    void execute(uint8_t byte);
    void clear();
    void collect(uint8_t byte);
    void param(uint8_t byte);
    void esc_dispatch(uint8_t byte);
    void csi_dispatch(uint8_t byte);
    void hook();
    void put(uint8_t byte);
    void unhook();
    void osc_start();
    void osc_put(uint8_t byte);
    void osc_end();

    void transition(State state);

    void on_input(uint8_t byte);

    TTYParserDispatcher& m_dispatcher;
    State m_last_state { State::Ground };
    State m_next_state { State::Ground };
    Function<void()> m_on_state_exit;

    String m_intermediate;
    String m_current_param;
    Vector<int> m_params;
};
