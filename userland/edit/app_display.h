#pragma once

#include <app/widget.h>
#include <edit/display.h>
#include <graphics/forward.h>
#include <liim/function.h>
#include <liim/string.h>
#include <liim/vector.h>

class AppDisplay;

class SearchWidget final : public App::Widget {
    APP_OBJECT(SearchWidget)

public:
    SearchWidget();
    virtual ~SearchWidget();

    virtual void render() override;

    AppDisplay& display();

private:
    SharedPtr<AppDisplay> m_display;
};

class AppDisplay final
    : public App::Widget
    , public Edit::Display {
    APP_OBJECT(AppDisplay)

public:
    virtual void initialize() override;
    virtual ~AppDisplay() override;

    virtual int rows() const override { return m_rows; }
    virtual int cols() const override { return m_cols; }

    constexpr int col_width() const { return 8; }
    constexpr int row_height() const { return 16; }

    virtual Edit::TextIndex text_index_at_mouse_position(const Point& point) override;
    virtual Edit::RenderedLine compose_line(const Edit::Line& line) override;
    virtual void output_line(int row, int col_offset, const Edit::RenderedLine& line, int line_index) override;
    virtual void invalidate_all_line_rects() override { invalidate(); }
    virtual void invalidate_line_rect(int row_in_display) override {
        invalidate(sized_rect().with_y(row_in_display * row_height()).with_height(row_height()));
    }
    virtual int enter() override;
    virtual void send_status_message(String message) override;
    virtual void enter_search(String starting_text) override;

    virtual App::ObjectBoundCoroutine quit() override;

    virtual void set_clipboard_contents(String text, bool is_whole_line) override;
    virtual String clipboard_contents(bool& is_whole_line) const override;

    virtual void render() override;

    Function<void()> on_quit;

private:
    struct CellData {
        char c;
        bool dirty;
        Edit::CharacterMetadata metadata;
    };

    explicit AppDisplay(bool m_main_display = true);

    AppDisplay& ensure_search_display();

    int index(int row, int col) const { return row * cols() + col; }

    void render_cursor(Renderer& renderer);
    void render_cell(Renderer& renderer, int x, int y, char c, Edit::CharacterMetadata metadata);

    int m_rows { 0 };
    int m_cols { 0 };
    int m_last_drawn_cursor_col { -1 };
    int m_last_drawn_cursor_row { -1 };
    bool m_main_display { false };
    SharedPtr<SearchWidget> m_search_widget;

    mutable String m_prev_clipboard_contents;
    mutable bool m_prev_clipboard_contents_were_whole_line { false };
};
