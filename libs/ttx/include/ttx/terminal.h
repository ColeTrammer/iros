#pragma once

#include "di/container/ring/ring.h"
#include "dius/sync_file.h"
#include "dius/tty.h"
#include "ttx/cursor_style.h"
#include "ttx/escape_sequence_parser.h"
#include "ttx/focus_event_io.h"
#include "ttx/graphics_rendition.h"
#include "ttx/key_event_io.h"
#include "ttx/mouse_event_io.h"
#include "ttx/params.h"

namespace ttx {
class Terminal {
public:
    struct Cell {
        c32 ch { ' ' };
        GraphicsRendition graphics_rendition;
        mutable bool dirty : 1 { true };
    };

    using Row = di::Vector<Cell>;

    explicit Terminal(dius::SyncFile& psuedo_terminal) : m_psuedo_terminal(psuedo_terminal) {}

    Terminal clone() const {
        auto result = Terminal(m_psuedo_terminal);
        result.m_rows = di::clone(m_rows);
        result.m_row_count = m_row_count;
        result.m_col_count = m_col_count;
        result.m_xpixels = m_xpixels;
        result.m_ypixels = m_ypixels;
        result.m_col_count = m_col_count;
        result.m_available_rows_in_display = m_available_rows_in_display;
        result.m_available_cols_in_display = m_available_cols_in_display;
        result.m_available_xpixels_in_display = m_available_xpixels_in_display;
        result.m_available_ypixels_in_display = m_available_ypixels_in_display;
        result.m_80_col_mode = m_80_col_mode;
        result.m_132_col_mode = m_132_col_mode;
        result.m_allow_80_132_col_mode = m_allow_80_132_col_mode;

        result.m_tab_stops = di::clone(m_tab_stops);
        result.m_cursor_row = m_cursor_row;
        result.m_cursor_col = m_cursor_col;
        result.m_cursor_style = m_cursor_style;
        result.m_saved_cursor_row = m_saved_cursor_row;
        result.m_saved_cursor_col = m_saved_cursor_col;
        result.m_cursor_hidden = m_cursor_hidden;
        result.m_disable_drawing = m_disable_drawing;
        result.m_autowrap_mode = m_autowrap_mode;
        result.m_x_overflow = m_x_overflow;
        result.m_origin_mode = m_origin_mode;

        result.m_application_cursor_keys_mode = m_application_cursor_keys_mode;
        result.m_key_reporting_flags = m_key_reporting_flags;
        result.m_key_reporting_flags_stack = di::clone(m_key_reporting_flags_stack);

        result.m_alternate_scroll_mode = m_alternate_scroll_mode;
        result.m_mouse_protocol = m_mouse_protocol;
        result.m_mouse_encoding = m_mouse_encoding;
        result.m_focus_event_mode = m_focus_event_mode;

        result.m_bracketed_paste = m_bracketed_paste;

        result.m_current_graphics_rendition = m_current_graphics_rendition;

        result.m_rows_below = di::clone(m_rows_below);
        result.m_rows_above = di::clone(m_rows_above);
        result.m_scroll_start = m_scroll_start;
        result.m_scroll_end = m_scroll_end;
        return result;
    }

    void on_parser_results(di::Span<ParserResult const> results);

    auto cursor_row() const -> u32 { return m_cursor_row; }
    auto cursor_col() const -> u32 { return m_cursor_col; }
    bool cursor_hidden() const { return m_cursor_hidden; }
    bool should_display_cursor_at_position(u32 r, u32 c) const;
    auto cursor_style() const -> CursorStyle { return m_cursor_style; }
    u32 scroll_relative_offset(u32 display_row) const;

    auto allowed_to_draw() const -> bool { return !m_disable_drawing; }

    void scroll_to_bottom();
    void scroll_up();
    void scroll_down();

    u32 total_rows() const { return m_rows_above.size() + m_rows.size() + m_rows_below.size(); }
    u32 row_offset() const { return m_rows_above.size(); }
    u32 row_count() const { return m_row_count; }
    u32 col_count() const { return m_col_count; }
    auto size() const -> dius::tty::WindowSize { return { m_row_count, m_col_count, m_xpixels, m_ypixels }; }

    void set_visible_size(dius::tty::WindowSize const& window_size);
    auto visible_size() const -> dius::tty::WindowSize {
        return { m_available_rows_in_display, m_available_cols_in_display, m_available_xpixels_in_display,
                 m_available_ypixels_in_display };
    }

    void scroll_down_if_needed();
    void scroll_up_if_needed();

    auto application_cursor_keys_mode() const -> ApplicationCursorKeysMode { return m_application_cursor_keys_mode; }
    auto key_reporting_flags() const -> KeyReportingFlags { return m_key_reporting_flags; }

    auto alternate_scroll_mode() const -> AlternateScrollMode { return m_alternate_scroll_mode; }
    auto mouse_protocol() const -> MouseProtocol { return m_mouse_protocol; }
    auto mouse_encoding() const -> MouseEncoding { return m_mouse_encoding; }
    auto in_alternate_screen_buffer() const -> bool { return !!m_save_state; }
    auto focus_event_mode() const -> FocusEventMode { return m_focus_event_mode; }

    auto bracked_paste() const -> bool { return m_bracketed_paste; }

    di::Vector<Row> const& rows() const { return m_rows; }
    Row const& row_at_scroll_relative_offset(u32 offset) const;

    void invalidate_all();

private:
    void on_parser_result(PrintableCharacter const& printable_character);
    void on_parser_result(DCS const& dcs);
    void on_parser_result(CSI const& csi);
    void on_parser_result(Escape const& escape);
    void on_parser_result(ControlCharacter const& control);

    void resize(dius::tty::WindowSize const& window_size);

    void put_char(u32 row, u32 col, c32 c);
    void put_char(c32 c);

    void save_pos() {
        m_saved_cursor_row = m_cursor_row;
        m_saved_cursor_col = m_cursor_col;
    }
    void restore_pos() {
        m_cursor_row = m_saved_cursor_row;
        m_cursor_col = m_saved_cursor_col;
    }

    void set_cursor(u32 row, u32 col);

    u32 min_row_inclusive() const {
        if (m_origin_mode) {
            return m_scroll_start;
        }
        return 0;
    }
    u32 min_col_inclusive() const { return 0; }

    u32 max_row_inclusive() const {
        if (m_origin_mode) {
            return m_scroll_end;
        }
        return m_row_count - 1;
    }
    u32 max_col_inclusive() const { return m_col_count - 1; }

    u32 translate_row(u32 row) const {
        if (m_origin_mode) {
            return row + m_scroll_start - 1;
        }
        return row - 1;
    }
    u32 translate_col(u32 col) const { return col - 1; }

    void clear_below_cursor(char ch = ' ');
    void clear_above_cursor(char ch = ' ');
    void clear(char ch = ' ');
    void clear_row(u32 row, char ch = ' ');
    void clear_row_until(u32 row, u32 end_col, char ch = ' ');
    void clear_row_to_end(u32 row, u32 start_col, char ch = ' ');

    void set_use_alternate_screen_buffer(bool b);

    void esc_decaln();
    void esc_decsc();
    void esc_decrc();

    void c0_bs();
    void c0_ht();
    void c0_lf();
    void c0_vt();
    void c0_ff();
    void c0_cr();

    void c1_ind();
    void c1_nel();
    void c1_hts();
    void c1_ri();

    void dcs_decrqss(Params const& params, di::StringView data);

    void csi_ich(Params const& params);
    void csi_cuu(Params const& params);
    void csi_cud(Params const& params);
    void csi_cuf(Params const& params);
    void csi_cub(Params const& params);
    void csi_cup(Params const& params);
    void csi_cha(Params const& params);
    void csi_ed(Params const& params);
    void csi_el(Params const& params);
    void csi_il(Params const& params);
    void csi_dl(Params const& params);
    void csi_dch(Params const& params);
    void csi_su(Params const& params);
    void csi_sd(Params const& params);
    void csi_ech(Params const& params);
    void csi_rep(Params const& params);
    void csi_da1(Params const& params);
    void csi_da2(Params const& params);
    void csi_da3(Params const& params);
    void csi_vpa(Params const& params);
    void csi_hvp(Params const& params);
    void csi_tbc(Params const& params);
    void csi_decset(Params const& params);
    void csi_decrst(Params const& params);
    void csi_decrqm(Params const& params);
    void csi_decscusr(Params const& params);
    void csi_sgr(Params const& params);
    void csi_dsr(Params const& params);
    void csi_decstbm(Params const& params);
    void csi_scosc(Params const& params);
    void csi_scorc(Params const& params);

    void csi_set_key_reporting_flags(Params const& params);
    void csi_get_key_reporting_flags(Params const& params);
    void csi_push_key_reporting_flags(Params const& params);
    void csi_pop_key_reporting_flags(Params const& params);

    di::Vector<Row> m_rows;
    u32 m_row_count { 0 };
    u32 m_col_count { 0 };
    u32 m_xpixels { 0 };
    u32 m_ypixels { 0 };
    u32 m_available_rows_in_display { 0 };
    u32 m_available_cols_in_display { 0 };
    u32 m_available_xpixels_in_display { 0 };
    u32 m_available_ypixels_in_display { 0 };
    bool m_80_col_mode { false };
    bool m_132_col_mode { false };
    bool m_allow_80_132_col_mode { false };

    di::Vector<u32> m_tab_stops;
    u32 m_cursor_row { 0 };
    u32 m_cursor_col { 0 };
    u32 m_saved_cursor_row { 0 };
    u32 m_saved_cursor_col { 0 };
    CursorStyle m_cursor_style { CursorStyle::SteadyBlock };
    bool m_cursor_hidden { false };
    bool m_disable_drawing { false };
    bool m_autowrap_mode { true };
    bool m_x_overflow { false };
    bool m_origin_mode { false };

    ApplicationCursorKeysMode m_application_cursor_keys_mode { ApplicationCursorKeysMode::Disabled };
    KeyReportingFlags m_key_reporting_flags { KeyReportingFlags::None };
    di::Ring<KeyReportingFlags> m_key_reporting_flags_stack;

    AlternateScrollMode m_alternate_scroll_mode { AlternateScrollMode::Disabled };
    MouseProtocol m_mouse_protocol { MouseProtocol::None };
    MouseEncoding m_mouse_encoding { MouseEncoding::X10 };
    FocusEventMode m_focus_event_mode { FocusEventMode::Disabled };

    bool m_bracketed_paste { false };

    GraphicsRendition m_current_graphics_rendition;

    di::Vector<Row> m_rows_below;
    di::Vector<Row> m_rows_above;
    u32 m_scroll_start { 0 };
    u32 m_scroll_end { 0 };

    di::Box<Terminal> m_save_state;

    dius::SyncFile& m_psuedo_terminal;
};
}
