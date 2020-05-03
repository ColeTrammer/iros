#pragma once

#include <liim/vector.h>

#include "panel.h"

class TerminalPanel final : public Panel {
public:
    TerminalPanel();
    virtual ~TerminalPanel() override;

    virtual int rows() const override { return m_rows; }
    virtual int cols() const override { return m_cols; }

    virtual void clear() override;
    virtual void set_text_at(int row, int col, char c) override;
    virtual void flush() override;
    virtual void set_cursor(int row, int col) override;

private:
    void draw_cursor();
    void print_char(char c);
    void flush_row(int line);

    int index(int row, int col) const { return row * m_cols + col; }

    Vector<char> m_chars;
    int m_rows { 0 };
    int m_cols { 0 };
    int m_cursor_row { 0 };
    int m_cursor_col { 0 };
};