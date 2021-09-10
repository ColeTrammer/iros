#pragma once

#include <graphics/color.h>
#include <liim/function.h>
#include <liim/maybe.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <terminal/forward.h>
#include <terminal/tty_parser.h>

namespace Terminal {
class TTY : public TTYParserDispatcher {
public:
    struct Cell {
        Maybe<Color> fg;
        Maybe<Color> bg;
        char ch { ' ' };
        bool bold : 1 { false };
        bool inverted : 1 { false };
        mutable bool dirty : 1 { true };
    };

    using Row = Vector<Cell>;

    TTY(PsuedoTerminal& psuedo_terminal) : m_psuedo_terminal(psuedo_terminal), m_parser(make_shared<TTYParser>(*this)) {}
    virtual ~TTY() override {}

    virtual void on_printable_character(uint8_t byte) override;
    virtual void on_csi(const String& intermediate, const Vector<int>& params, uint8_t terminator) override;
    virtual void on_escape(const String& intermediate, uint8_t terminator) override;
    virtual void on_c0_character(uint8_t byte) override;

    int cursor_row() const { return m_cursor_row; }
    int cursor_col() const { return m_cursor_col; }
    bool cursor_hidden() const { return m_cursor_hidden; }
    bool should_display_cursor_at_position(int r, int c) const;
    int scroll_relative_offset(int display_row) const;

    void scroll_to_bottom();
    void scroll_up();
    void scroll_down();

    int total_rows() const { return m_rows_above.size() + m_rows.size() + m_rows_below.size(); }
    int row_offset() const { return m_rows_above.size(); }
    int row_count() const { return m_row_count; }
    int col_count() const { return m_col_count; }

    void set_visible_size(int rows, int cols);
    void on_input(Span<const uint8_t> input);

    int available_rows_in_display() const { return m_available_rows_in_display; }
    int available_cols_in_display() const { return m_available_cols_in_display; }

    void scroll_down_if_needed();
    void scroll_up_if_needed();

    const Vector<Row>& rows() const { return m_rows; }
    const Row& row_at_scroll_relative_offset(int offset) const;

    void invalidate_all();

private:
    void resize(int rows, int cols);

    void put_char(int row, int col, char c);
    void put_char(char c);

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

    void reset_bg() { m_bg.reset(); }
    void reset_fg() { m_fg.reset(); }

    void set_bg(Maybe<Color> c) { m_bg = c; }
    void set_fg(Maybe<Color> c) { m_fg = c; }

    void set_inverted(bool b) { m_inverted = b; }
    void set_bold(bool b) { m_bold = b; }
    void set_use_alternate_screen_buffer(bool b);

    void reset_attributes() {
        reset_bg();
        reset_fg();
        set_inverted(false);
        set_bold(false);
    }

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

    void csi_ich(const Vector<int>& params);
    void csi_cuu(const Vector<int>& params);
    void csi_cud(const Vector<int>& params);
    void csi_cuf(const Vector<int>& params);
    void csi_cub(const Vector<int>& params);
    void csi_cup(const Vector<int>& params);
    void csi_cha(const Vector<int>& params);
    void csi_ed(const Vector<int>& params);
    void csi_el(const Vector<int>& params);
    void csi_il(const Vector<int>& params);
    void csi_dl(const Vector<int>& params);
    void csi_dch(const Vector<int>& params);
    void csi_su(const Vector<int>& params);
    void csi_sd(const Vector<int>& params);
    void csi_ech(const Vector<int>& params);
    void csi_rep(const Vector<int>& params);
    void csi_da1(const Vector<int>& params);
    void csi_da2(const Vector<int>& params);
    void csi_da3(const Vector<int>& params);
    void csi_vpa(const Vector<int>& params);
    void csi_hvp(const Vector<int>& params);
    void csi_tbc(const Vector<int>& params);
    void csi_decset(const Vector<int>& params);
    void csi_decrst(const Vector<int>& params);
    void csi_sgr(const Vector<int>& params);
    void csi_dsr(const Vector<int>& params);
    void csi_decstbm(const Vector<int>& params);
    void csi_scosc(const Vector<int>& params);
    void csi_scorc(const Vector<int>& params);

    Vector<Row> m_rows;
    int m_row_count { 0 };
    int m_col_count { 0 };
    int m_available_rows_in_display { 0 };
    int m_available_cols_in_display { 0 };
    bool m_80_col_mode { false };
    bool m_132_col_mode { false };
    bool m_allow_80_132_col_mode { false };

    Vector<int> m_tab_stops;
    int m_cursor_row { 0 };
    int m_cursor_col { 0 };
    int m_saved_cursor_row { 0 };
    int m_saved_cursor_col { 0 };
    bool m_cursor_hidden { false };
    bool m_autowrap_mode { true };
    bool m_x_overflow { false };
    bool m_origin_mode { false };

    bool m_inverted { false };
    bool m_bold { false };
    Maybe<Color> m_fg;
    Maybe<Color> m_bg;

    Vector<Row> m_rows_below;
    Vector<Row> m_rows_above;
    int m_scroll_start { 0 };
    int m_scroll_end { 0 };

    SharedPtr<TTY> m_save_state;

    PsuedoTerminal& m_psuedo_terminal;
    SharedPtr<TTYParser> m_parser;
};
}
