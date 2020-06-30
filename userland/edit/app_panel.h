#pragma once

#include <app/widget.h>
#include <liim/bitmap.h>
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

    virtual void notify_now_is_a_good_time_to_draw_cursor() override;
    virtual void notify_line_count_changed() override;

    virtual void set_clipboard_contents(String text, bool is_whole_line) override;
    virtual String clipboard_contents(bool& is_whole_line) const override;

    virtual void set_cursor(int row, int col) override;

    virtual int cursor_col() const { return m_cursor_col; }
    virtual int cursor_row() const { return m_cursor_row; }

    virtual void do_open_prompt() override;

    virtual void render() override;
    virtual void on_key_event(App::KeyEvent& event) override;
    virtual void on_resize() override;
    virtual void on_focused() override;

private:
    struct CellData {
        char c;
        bool dirty;
        CharacterMetadata metadata;
    };

    AppPanel(bool m_main_panel = true);

    virtual void document_did_change() override;

    AppPanel& ensure_search_panel();

    int index(int row, int col) const { return row * cols() + col; }

    void render_cursor(Renderer& renderer);
    void render_cell(Renderer& renderer, int x, int y, CellData& cell);

    int m_rows { 0 };
    int m_cols { 0 };
    int m_last_drawn_cursor_col { -1 };
    int m_last_drawn_cursor_row { -1 };
    int m_cursor_col { 0 };
    int m_cursor_row { 0 };
    bool m_cursor_dirty { true };
    bool m_main_panel { false };
    Vector<CellData> m_cells;
    SharedPtr<AppPanel> m_search_panel;
};
