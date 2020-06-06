#pragma once

#include <app/widget.h>
#include <liim/string.h>
#include <liim/vector.h>

#include "panel.h"

class Renderer;

class AppPanel final
    : public Panel
    , public App::Widget {
    APP_OBJECT(AppPanel)

public:
    virtual ~AppPanel() override;

    virtual int rows() const override { return m_rows; }
    virtual int cols() const override { return m_cols; };

    constexpr int col_width() const { return 8; }
    constexpr int row_height() const { return 16; }

    virtual void clear() override;
    virtual void set_text_at(int row, int col, char c, CharacterMetadata metadata) override;
    virtual void flush() override;
    virtual void enter() override;
    virtual void send_status_message(String message) override;
    virtual String prompt(const String& message) override;
    virtual void enter_search(String starting_text) override;
    virtual void notify_line_count_changed() override;

    virtual void set_clipboard_contents(String text, bool is_whole_line) override;
    virtual String clipboard_contents(bool& is_whole_line) const override;

    virtual void set_cursor(int row, int col) override;

    virtual int cursor_col() const { return m_cursor_col; }
    virtual int cursor_row() const { return m_cursor_row; }

    virtual void do_open_prompt() override;

    virtual void render() override;

private:
    struct CellData {
        char c;
        CharacterMetadata metadata;
    };

    AppPanel(int x, int y, int width, int height);

    virtual void document_did_change() override;

    int index(int row, int col) const { return row * cols() + col; }

    void render_cell(Renderer& renderer, int x, int y, CellData& cell);
    void update_coordinates(int x, int y, int width, int height);

    int m_rows { 0 };
    int m_cols { 0 };
    int m_cursor_col { 0 };
    int m_cursor_row { 0 };
    Vector<CellData> m_cells;
};
