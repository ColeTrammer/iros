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

    virtual Edit::RenderedLine compose_line(const Edit::Line& line) const override;
    virtual void output_line(int row, int col_offset, const StringView& text, const Vector<Edit::CharacterMetadata>& metadata) override;
    virtual void schedule_update() override;
    virtual int enter() override;
    virtual void send_status_message(String message) override;
    virtual Maybe<String> prompt(const String& message) override;
    virtual void enter_search(String starting_text) override;
    virtual void quit() override;

    virtual void set_clipboard_contents(String text, bool is_whole_line) override;
    virtual String clipboard_contents(bool& is_whole_line) const override;

    virtual void do_open_prompt() override;

    virtual void render() override;
    virtual void on_key_event(const App::KeyEvent& event) override;
    virtual void on_mouse_event(const App::MouseEvent& event) override;
    virtual void on_resize() override;
    virtual void on_focused() override;

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
    void render_cell(Renderer& renderer, int x, int y, char c, Edit::CharacterMetadata metadata);

    int m_rows { 0 };
    int m_cols { 0 };
    int m_last_drawn_cursor_col { -1 };
    int m_last_drawn_cursor_row { -1 };
    bool m_main_panel { false };
    SharedPtr<SearchWidget> m_search_widget;

    mutable String m_prev_clipboard_contents;
    mutable bool m_prev_clipboard_contents_were_whole_line { false };
};
