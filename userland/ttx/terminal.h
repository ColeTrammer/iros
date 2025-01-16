#pragma once

#include "dius/sync_file.h"
#include "escape_sequence_parser.h"
#include "graphics_rendition.h"
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
        result.m_available_rows_in_display = m_available_rows_in_display;
        result.m_available_cols_in_display = m_available_cols_in_display;
        result.m_80_col_mode = m_80_col_mode;
        result.m_132_col_mode = m_132_col_mode;
        result.m_allow_80_132_col_mode = m_allow_80_132_col_mode;

        result.m_tab_stops = di::clone(m_tab_stops);
        result.m_cursor_row = m_cursor_row;
        result.m_cursor_col = m_cursor_col;
        result.m_saved_cursor_row = m_saved_cursor_row;
        result.m_saved_cursor_col = m_saved_cursor_col;
        result.m_cursor_hidden = m_cursor_hidden;
        result.m_disable_drawing = m_disable_drawing;
        result.m_autowrap_mode = m_autowrap_mode;
        result.m_x_overflow = m_x_overflow;
        result.m_origin_mode = m_origin_mode;

        result.m_current_graphics_rendition = m_current_graphics_rendition;

        result.m_rows_below = di::clone(m_rows_below);
        result.m_rows_above = di::clone(m_rows_above);
        result.m_scroll_start = m_scroll_start;
        result.m_scroll_end = m_scroll_end;
        return result;
    }

    void on_parser_results(di::Span<ParserResult const> results);

    int cursor_row() const { return m_cursor_row; }
    int cursor_col() const { return m_cursor_col; }
    bool cursor_hidden() const { return m_cursor_hidden; }
    bool should_display_cursor_at_position(int r, int c) const;
    int scroll_relative_offset(int display_row) const;

    auto allowed_to_draw() const -> bool { return !m_disable_drawing; }

    void scroll_to_bottom();
    void scroll_up();
    void scroll_down();

    int total_rows() const { return m_rows_above.size() + m_rows.size() + m_rows_below.size(); }
    int row_offset() const { return m_rows_above.size(); }
    int row_count() const { return m_row_count; }
    int col_count() const { return m_col_count; }

    void set_visible_size(int rows, int cols);

    int available_rows_in_display() const { return m_available_rows_in_display; }
    int available_cols_in_display() const { return m_available_cols_in_display; }

    void scroll_down_if_needed();
    void scroll_up_if_needed();

    di::Vector<Row> const& rows() const { return m_rows; }
    Row const& row_at_scroll_relative_offset(int offset) const;

    void invalidate_all();

private:
    void on_parser_result(PrintableCharacter const& printable_character);
    void on_parser_result(DCS const& dcs);
    void on_parser_result(CSI const& csi);
    void on_parser_result(Escape const& escape);
    void on_parser_result(ControlCharacter const& control);

    void resize(int rows, int cols);

    void put_char(int row, int col, c32 c);
    void put_char(c32 c);

    void save_pos() {
        m_saved_cursor_row = m_cursor_row;
        m_saved_cursor_col = m_cursor_col;
    }
    void restore_pos() {
        m_cursor_row = m_saved_cursor_row;
        m_cursor_col = m_saved_cursor_col;
    }

    void set_cursor(int row, int col);

    int min_row_inclusive() const {
        if (m_origin_mode) {
            return m_scroll_start;
        }
        return 0;
    }
    int min_col_inclusive() const { return 0; }

    int max_row_inclusive() const {
        if (m_origin_mode) {
            return m_scroll_end;
        }
        return m_row_count - 1;
    }
    int max_col_inclusive() const { return m_col_count - 1; }

    int translate_row(int row) const {
        if (m_origin_mode) {
            return row + m_scroll_start - 1;
        }
        return row - 1;
    }
    int translate_col(int col) const { return col - 1; }

    void clear_below_cursor(char ch = ' ');
    void clear_above_cursor(char ch = ' ');
    void clear(char ch = ' ');
    void clear_row(int row, char ch = ' ');
    void clear_row_until(int row, int end_col, char ch = ' ');
    void clear_row_to_end(int row, int start_col, char ch = ' ');

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
    void csi_sgr(Params const& params);
    void csi_dsr(Params const& params);
    void csi_decstbm(Params const& params);
    void csi_scosc(Params const& params);
    void csi_scorc(Params const& params);

    di::Vector<Row> m_rows;
    int m_row_count { 0 };
    int m_col_count { 0 };
    int m_available_rows_in_display { 0 };
    int m_available_cols_in_display { 0 };
    bool m_80_col_mode { false };
    bool m_132_col_mode { false };
    bool m_allow_80_132_col_mode { false };

    di::Vector<int> m_tab_stops;
    int m_cursor_row { 0 };
    int m_cursor_col { 0 };
    int m_saved_cursor_row { 0 };
    int m_saved_cursor_col { 0 };
    bool m_cursor_hidden { false };
    bool m_disable_drawing { false };
    bool m_autowrap_mode { true };
    bool m_x_overflow { false };
    bool m_origin_mode { false };

    GraphicsRendition m_current_graphics_rendition;

    di::Vector<Row> m_rows_below;
    di::Vector<Row> m_rows_above;
    int m_scroll_start { 0 };
    int m_scroll_end { 0 };

    di::Box<Terminal> m_save_state;

    dius::SyncFile& m_psuedo_terminal;
};
}
