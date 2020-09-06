#pragma once

#include <app/widget.h>
#include <liim/bitmap.h>
#include <liim/function.h>
#include <liim/string.h>
#include <liim/vector.h>

#include "panel.h"

class Renderer;

class AppPanel;

class SearchWidget final : public App::Widget {
    APP_OBJECT(SearchWidget)

public:
    SearchWidget();
    virtual ~SearchWidget();

    virtual void render() override;

    AppPanel& panel();

private:
    SharedPtr<AppPanel> m_panel;
};

class AppPanel final
    : public Panel
    , public App::Widget {
    APP_OBJECT(AppPanel)

public:
    virtual void initialize() override;
    virtual ~AppPanel() override;

    virtual int rows() const override { return m_rows; }
    virtual int cols() const override { return m_cols; };

    constexpr int col_width() const { return 8; }
    constexpr int row_height() const { return 16; }

    virtual void clear() override;
    virtual void set_text_at(int row, int col, char c, CharacterMetadata metadata) override;
    virtual void flush() override;
    virtual int enter() override;
    virtual void send_status_message(String message) override;
    virtual Maybe<String> prompt(const String& message) override;
    virtual void enter_search(String starting_text) override;
    virtual void quit() override;

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
    virtual void on_mouse_event(App::MouseEvent& event) override;
    virtual void on_resize() override;
    virtual void on_focused() override;

    Function<void()> on_quit;

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

    int index_into_line_at_position(int wx, int wy) const;
    int index_of_line_at_position(int wx, int wy) const;

    int m_rows { 0 };
    int m_cols { 0 };
    int m_last_drawn_cursor_col { -1 };
    int m_last_drawn_cursor_row { -1 };
    int m_cursor_col { 0 };
    int m_cursor_row { 0 };
    bool m_cursor_dirty { true };
    bool m_main_panel { false };
    bool m_mouse_left_down { false };
    bool m_mouse_right_down { false };
    Vector<CellData> m_cells;
    SharedPtr<SearchWidget> m_search_widget;

    mutable String m_prev_clipboard_contents;
    mutable bool m_prev_clipboard_contents_were_whole_line { false };
};
