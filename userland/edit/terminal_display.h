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

class TerminalDisplay final
    : public Edit::Display
    , public TUI::Panel {
    APP_OBJECT(TerminalDisplay)

public:
    TerminalDisplay();
    virtual ~TerminalDisplay() override;

    // ^TUI::Panel
    virtual void render() override;
    virtual Maybe<Point> cursor_position() override;
    virtual void on_mouse_event(const App::MouseEvent& event) override;
    virtual void on_key_event(const App::KeyEvent& event) override;
    virtual void on_resize() override;
    virtual void on_made_active() override;

    // ^Edit::Display
    virtual int rows() const override { return sized_rect().height(); }
    virtual int cols() const override;

    virtual Edit::TextIndex text_index_at_mouse_position(const Point& point) override;
    virtual Edit::RenderedLine compose_line(const Edit::Line& line) override;
    virtual void output_line(int row, int col_offset, const StringView& text, const Vector<Edit::CharacterMetadata>& metadata) override;
    virtual void schedule_update() override { invalidate(); }
    virtual int enter() override;
    virtual void send_status_message(String message) override;
    virtual Maybe<String> prompt(const String& message) override;
    virtual void enter_search(String starting_text) override;
    virtual void notify_line_count_changed() override;
    virtual void quit() override;
    virtual void do_open_prompt() override;

    virtual void set_clipboard_contents(String text, bool is_whole_line) override;
    virtual String clipboard_contents(bool& is_whole_line) const override;

private:
    virtual void document_did_change() override;

    void compute_cols_needed_for_line_numbers();

    mutable String m_prev_clipboard_contents;
    mutable bool m_prev_clipboard_contents_were_whole_line { false };
    int m_cols_needed_for_line_numbers { 0 };
};
