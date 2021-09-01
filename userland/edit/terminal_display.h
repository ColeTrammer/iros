#pragma once

#include <edit/display.h>
#include <eventloop/event.h>
#include <liim/hash_map.h>
#include <liim/maybe.h>
#include <liim/variant.h>
#include <liim/vector.h>
#include <time.h>
#include <tinput/terminal_input_parser.h>
#include <tui/panel.h>

class TerminalPrompt;
class TerminalSearch;

class TerminalDisplay final
    : public Edit::Display
    , public TUI::Panel {
    APP_OBJECT(TerminalDisplay)

public:
    TerminalDisplay();
    virtual void initialize() override;
    virtual ~TerminalDisplay() override;

    // ^TUI::Panel
    virtual bool steals_focus() override { return m_steals_focus; };
    virtual void render() override;
    virtual Maybe<Point> cursor_position() override;

    // ^Edit::Display
    virtual int rows() const override { return sized_rect().height(); }
    virtual int cols() const override;

    virtual App::Object& this_widget() override { return *this; }
    virtual Edit::TextIndex text_index_at_mouse_position(const Point& point) override;
    virtual Edit::RenderedLine compose_line(const Edit::Line& line) override;
    virtual void output_line(int row, int col_offset, const Edit::RenderedLine& line, int line_index) override;
    virtual void invalidate_all_line_rects() override { invalidate(); }
    virtual void invalidate_line_rect(int row_in_display) override { invalidate(sized_rect().with_y(row_in_display).with_height(1)); }
    virtual int enter() override;
    virtual void send_status_message(String message) override;
    virtual Task<Maybe<String>> prompt(String message) override;
    virtual void enter_search(String starting_text) override;
    virtual void quit() override;
    virtual void do_open_prompt() override;

    virtual void set_clipboard_contents(String text, bool is_whole_line) override;
    virtual String clipboard_contents(bool& is_whole_line) const override;

    void set_steals_focus(bool b) { m_steals_focus = b; }

    void hide_prompt_panel();
    void hide_search_panel();

private:
    virtual void document_did_change() override;

    void compute_cols_needed_for_line_numbers();

    mutable String m_prev_clipboard_contents;
    mutable bool m_prev_clipboard_contents_were_whole_line { false };
    int m_cols_needed_for_line_numbers { 0 };
    SharedPtr<TerminalPrompt> m_prompt_panel;
    SharedPtr<TerminalSearch> m_search_panel;
    bool m_steals_focus { false };
};
