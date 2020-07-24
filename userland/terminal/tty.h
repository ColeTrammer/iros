#pragma once

#include <graphics/color.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <liim/vector.h>

class PsuedoTerminal;

class TTY {
public:
    struct Cell {
        Color fg { ColorValue::LightGray };
        Color bg { ColorValue::Black };
        char ch { ' ' };
        bool bold { false };
        bool inverted { false };
        mutable bool dirty { true };
    };

    using Row = Vector<Cell>;

    TTY(PsuedoTerminal& psuedo_terminal) : m_psuedo_terminal(psuedo_terminal) {}

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

    void resize(int rows, int cols);
    void on_char(char c);

    void scroll_down_if_needed();

    const Vector<Row>& rows() const { return m_rows; }
    const Row& row_at_scroll_relative_offset(int offset) const;

private:
    void save_pos() {
        m_saved_cursor_row = m_cursor_row;
        m_saved_cursor_col = m_cursor_col;
    }
    void restore_pos() {
        m_cursor_row = m_saved_cursor_row;
        m_cursor_col = m_saved_cursor_col;
    }

    void clear_below_cursor(char ch = ' ');
    void clear_above_cursor(char ch = ' ');
    void clear(char ch = ' ');
    void clear_row(int row, char ch = ' ');
    void clear_row_until(int row, int end_col, char ch = ' ');
    void clear_row_to_end(int row, int start_col, char ch = ' ');

    void invalidate_all();

    void reset_bg() { m_bg = ColorValue::Black; }
    void reset_fg() { m_fg = ColorValue::LightGray; }

    void set_bg(Color c) { m_bg = c; }
    void set_fg(Color c) { m_fg = c; }

    void set_inverted(bool b) { m_inverted = b; }
    void set_bold(bool b) { m_bold = b; }
    void set_use_alternate_screen_buffer(bool b);

    void reset_attributes() {
        reset_bg();
        reset_fg();
        set_inverted(false);
        set_bold(false);
    }

    void put_char(int row, int col, char c);
    void put_char(char c);

    void handle_escape_sequence();
    void on_next_escape_char(char);

    void clamp_cursor();

    int m_row_count { 0 };
    int m_col_count { 0 };
    int m_cursor_row { 0 };
    int m_cursor_col { 0 };
    bool m_inverted { false };
    bool m_bold { false };
    Color m_fg { ColorValue::LightGray };
    Color m_bg { ColorValue::Black };
    Vector<Row> m_rows;
    Vector<Row> m_rows_below;
    Vector<Row> m_rows_above;
    bool m_cursor_hidden { false };
    bool m_in_escape { false };
    bool m_x_overflow { false };
    int m_saved_cursor_row { 0 };
    int m_saved_cursor_col { 0 };
    int m_escape_index { 0 };
    int m_scroll_start { 0 };
    int m_scroll_end { 0 };
    char m_escape_buffer[50] { 0 };
    PsuedoTerminal& m_psuedo_terminal;
    SharedPtr<TTY> m_save_state;
};
