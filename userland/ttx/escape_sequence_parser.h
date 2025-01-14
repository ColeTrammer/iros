#pragma once

#include "di/container/string/string.h"
#include "di/container/string/string_view.h"
#include "di/function/container/function.h"
#include "di/reflect/prelude.h"
#include "di/vocab/variant/prelude.h"

namespace ttx {
struct PrintableCharacter {
    c32 code_point = 0;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<PrintableCharacter>) {
        return di::make_fields(di::field<"code_point", &PrintableCharacter::code_point>);
    }
};

struct DCS {
    di::String intermediate;
    di::Vector<i32> params;
    di::String data;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<DCS>) {
        return di::make_fields(di::field<"intermediate", &DCS::intermediate>, di::field<"params", &DCS::params>,
                               di::field<"data", &DCS::data>);
    }
};

struct CSI {
    di::String intermediate;
    di::Vector<i32> params;
    c32 terminator = 0;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CSI>) {
        return di::make_fields(di::field<"intermediate", &CSI::intermediate>, di::field<"params", &CSI::params>,
                               di::field<"terminator", &CSI::terminator>);
    }
};

struct Escape {
    di::String intermediate;
    c32 terminator = 0;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<Escape>) {
        return di::make_fields(di::field<"intermediate", &Escape::intermediate>,
                               di::field<"terminator", &Escape::terminator>);
    }
};

struct ControlCharacter {
    i32 code_point = 0; // Not a c32, so that it will be printed as a decimal.

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ControlCharacter>) {
        return di::make_fields(di::field<"code_point", &ControlCharacter::code_point>);
    }
};

using ParserResult = di::Variant<PrintableCharacter, DCS, CSI, Escape, ControlCharacter>;

class EscapeSequenceParser {
public:
    auto parse(di::StringView data) -> di::Vector<ParserResult>;

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

#define __ENUMERATE_STATE(N, n) void n##_state(c32 code_point);
    __ENUMERATE_STATES(__ENUMERATE_STATE)
#undef __ENUMERATE_STATE

    void ignore(c32 code_point);
    void print(c32 code_point);
    void execute(c32 code_point);
    void clear();
    void collect(c32 code_point);
    void param(c32 code_point);
    void esc_dispatch(c32 code_point);
    void csi_dispatch(c32 code_point);
    void hook();
    void put(c32 code_point);
    void unhook();
    void osc_start();
    void osc_put(c32 code_point);
    void osc_end();

    void transition(State state);

    void on_input(c32 code_point);

    State m_last_state { State::Ground };
    State m_next_state { State::Ground };
    di::Function<void()> m_on_state_exit;

    di::String m_intermediate;
    di::String m_current_param;
    di::String m_data;
    di::Vector<i32> m_params;
    di::Vector<ParserResult> m_result;
};
}
