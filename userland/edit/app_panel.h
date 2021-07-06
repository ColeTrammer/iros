#pragma once

#include <app/widget.h>
#include <edit/panel.h>
#include <graphics/forward.h>
#include <liim/function.h>
#include <liim/string.h>
#include <liim/vector.h>

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
    : public Edit::Panel
    , public App::Widget {
    APP_OBJECT(AppPanel)

public:
    virtual void initialize() override;
    virtual ~AppPanel() override;

    virtual int rows() const override { return m_rows; }
    virtual int cols() const override { return m_cols; }

    constexpr int col_width() const { return 8; }
    constexpr int row_height() const { return 16; }

    virtual void clear() override;
    virtual void set_text_at(int row, int col, char c, Edit::CharacterMetadata metadata) override;
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

    virtual void do_open_prompt() override;

    virtual void render() override;
    virtual void on_key_event(const App::KeyEvent& event) override;
    virtual void on_mouse_event(const App::MouseEvent& event) override;
    virtual void on_resize() override;
    virtual void on_focused() override;
    virtual void on_theme_change_event(const App::ThemeChangeEvent&) override {
        for (auto& c : m_cells) {
            c.dirty = true;
        }
    }

    Function<void()> on_quit;

private:
    struct CellData {
        char c;
        bool dirty;
        Edit::CharacterMetadata metadata;
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
    bool m_main_panel { false };
    Vector<CellData> m_cells;
    SharedPtr<SearchWidget> m_search_widget;

    mutable String m_prev_clipboard_contents;
    mutable bool m_prev_clipboard_contents_were_whole_line { false };
};
